/****************************************************************************
**
** Copyright (C) 2026 Johannes Meyer zum Alten Borgloh
**
** This file is part of Waydroid Runner.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of the copyright holder nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

#include "setup.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

#include <iostream>

namespace {

typedef QMap<QString, QString> StringMap;

const QString service = QStringLiteral("id.waydro.Container");
const QString path = QStringLiteral("/Initializer");
const QString interface = QStringLiteral("id.waydro.Initializer");

// Waydroid's work directory and the channels it downloads from by default
// (tools/config/__init__.py)
const QString work = QStringLiteral("/var/lib/waydroid");
const QString channelsConfig = QStringLiteral("/usr/share/waydroid-extra/channels.cfg");

// About 1 GB of zips; the system image unpacks to about 2 GB while its zip
// is still there
const qint64 needed = 3000LL * 1000 * 1000;

// channels.cfg as Waydroid reads it (Python's configparser): section
// [channels], "key = value" or "key: value", keys in lower case, values
// stripped; Waydroid's defaults for what is missing. Init asks for no
// authorization only if the channels equal these values.
StringMap readChannels()
{
  StringMap c;
  c[QStringLiteral("system_channel")] = QStringLiteral("https://ota.waydro.id/system");
  c[QStringLiteral("vendor_channel")] = QStringLiteral("https://ota.waydro.id/vendor");
  c[QStringLiteral("system_type")] = QStringLiteral("VANILLA");

  QFile f(channelsConfig);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return c;

  QTextStream in(&f);
  bool section = false;
  while (!in.atEnd())
    {
      QString line = in.readLine().trimmed();
      if (line.isEmpty() || line.startsWith('#') || line.startsWith(';'))
        continue;
      if (line.startsWith('['))
        {
          section = (line == QLatin1String("[channels]"));
          continue;
        }
      if (!section)
        continue;
      int eq = line.indexOf('='), colon = line.indexOf(':');
      int d = eq < 0 ? colon : (colon < 0 ? eq : qMin(eq, colon));
      if (d <= 0)
        continue;
      QString key = line.left(d).trimmed().toLower();
      if (c.contains(key))
        c[key] = line.mid(d + 1).trimmed();
    }
  return c;
}

}

Setup::Setup(QObject *parent):
  QObject(parent)
{
  qDBusRegisterMetaType<StringMap>();
  updateSpace();
}

bool Setup::initialized()
{
  return QFileInfo(work + QStringLiteral("/waydroid.cfg")).isFile() &&
      QFileInfo(work + QStringLiteral("/rootfs")).isDir();
}

qint64 Setup::bytesNeeded() const
{
  return needed;
}

void Setup::updateSpace()
{
  // /var/lib/waydroid may be a link to another file system (/home)
  QString dir = QFileInfo(work).canonicalFilePath();
  qint64 free = -1;
  if (!dir.isEmpty())
    {
      QStorageInfo storage(dir);
      if (storage.isValid())
        free = storage.bytesAvailable();
    }
  if (free != m_bytes_free)
    {
      m_bytes_free = free;
      emit spaceChanged();
    }
}

void Setup::start()
{
  if (m_state == State::Running)
    return;

  // Before the call: the progress begins at once
  QDBusConnection bus = QDBusConnection::systemBus();
  if (!m_signals_connected)
    m_signals_connected =
        bus.connect(service, path, interface, QStringLiteral("ProgressChanged"), this, SLOT(onProgress(QString))) &&
        bus.connect(service, path, interface, QStringLiteral("Finished"), this, SLOT(onFinished())) &&
        bus.connect(service, path, interface, QStringLiteral("Interrupted"), this, SLOT(onInterrupted()));
  if (!m_signals_connected)
    std::cerr << "[Setup] Cannot follow Waydroid's setup signals: "
              << bus.lastError().message().toStdString() << std::endl;

  StringMap channels = readChannels();
  StringMap params;
  params[QStringLiteral("system_channel")] = channels.value(QStringLiteral("system_channel"));
  params[QStringLiteral("vendor_channel")] = channels.value(QStringLiteral("vendor_channel"));
  params[QStringLiteral("system_type")] = channels.value(QStringLiteral("system_type"));
  std::cout << "[Setup] Init: " << params.value(QStringLiteral("system_channel")).toStdString() << " "
            << params.value(QStringLiteral("vendor_channel")).toStdString() << " "
            << params.value(QStringLiteral("system_type")).toStdString() << std::endl;

  m_cancel_requested = false;
  m_failure.clear();
  setStep(tr("Preparing"));
  setState(State::Running);

  QDBusMessage call = QDBusMessage::createMethodCall(service, path, interface, QStringLiteral("Init"));
  call << QVariant::fromValue(params);
  QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(bus.asyncCall(call), this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this, &Setup::onInitReply);
}

void Setup::cancel()
{
  if (m_state != State::Running || m_cancel_requested)
    return;

  // Waydroid stops its setup process; Interrupted follows
  m_cancel_requested = true;
  setStep(tr("Cancelling"));
  QDBusMessage call = QDBusMessage::createMethodCall(service, path, interface, QStringLiteral("Cancel"));
  QDBusConnection::systemBus().asyncCall(call);
  QTimer::singleShot(10000, this, [this]() {
      if (m_state == State::Running && m_cancel_requested)
        setState(State::Cancelled);
    });
}

void Setup::onInitReply(QDBusPendingCallWatcher *watcher)
{
  QDBusPendingReply<> reply = *watcher;
  watcher->deleteLater();
  if (!reply.isError() || m_state != State::Running)
    return;

  QDBusError e = reply.error();
  std::cerr << "[Setup] Init failed: " << e.name().toStdString() << ": " << e.message().toStdString() << std::endl;
  // D-Bus activation starts the service (through systemd). When its start
  // fails, the call only times out (systemd 238 does not report it).
  if (e.type() == QDBusError::ServiceUnknown)
    setState(State::Failed, tr("Waydroid's service is not available."), e.message());
  else if (e.type() == QDBusError::NoReply || e.type() == QDBusError::Timeout ||
           e.type() == QDBusError::TimedOut || e.type() == QDBusError::Disconnected ||
           e.name().startsWith(QLatin1String("org.freedesktop.DBus.Error.Spawn")))
    setState(State::Failed, tr("Waydroid's service did not start."), e.message());
  else if (e.message().contains(QLatin1String("Polkit")))
    setState(State::Failed, tr("Waydroid refused the download channels."), e.message());
  else
    setState(State::Failed, tr("Waydroid could not start the setup."), e.message());
}

void Setup::onProgress(const QString &message)
{
  // Waydroid's setup sends every write to its output and every log line of
  // INFO and above. The download progress is printed with a carriage return
  // every 2 seconds.
  if (m_state != State::Running)
    return;

  QString text = message;
  text.remove('\r');
  text = text.trimmed();
  if (text.isEmpty())
    return;
  std::cout << "[Setup] " << text.toStdString() << std::endl;

  static const QRegularExpression downloading(
        QStringLiteral("^\\[Downloading\\]\\s+([0-9.]+) MB/([0-9.]+) MB\\s+([0-9.]+) (kB/s|MB/s)"));
  QRegularExpressionMatch m = downloading.match(text);
  if (m.hasMatch())
    {
      double done = m.captured(1).toDouble();
      double total = m.captured(2).toDouble();
      if (total > 0 && !m_cancel_requested)
        setStep(m_step, done / total,
                tr("%1 of %2 MB, %3 %4").arg(qRound(done)).arg(qRound(total))
                .arg(m.captured(3)).arg(m.captured(4)));
      return;
    }

  if (m_cancel_requested)
    return;

  if (text.startsWith(QLatin1String("Downloading ")))
    setStep(text.contains(QLatin1String("vendor")) ? tr("Downloading Android, part 2 of 2")
                                                   : tr("Downloading Android, part 1 of 2"), 0);
  else if (text.startsWith(QLatin1String("Validating ")))
    setStep(tr("Checking the download"));
  else if (text.startsWith(QLatin1String("Extracting ")))
    setStep(tr("Unpacking"));
  else if (text.startsWith(QLatin1String("Exception during init")) || text.contains(QLatin1String("Traceback")))
    {
      // The last line names the error
      QStringList lines = text.split('\n', QString::SkipEmptyParts);
      m_failure = lines.isEmpty() ? text : lines.last().trimmed();
    }
}

void Setup::onFinished()
{
  if (m_state != State::Running)
    return;

  std::cout << "[Setup] Android is set up" << std::endl;
  updateSpace();
  setStep(tr("Android is set up"), 1);
  setState(State::Done);
  emit finished();
}

void Setup::onInterrupted()
{
  if (m_state != State::Running)
    return;

  updateSpace();
  if (m_cancel_requested)
    {
      std::cout << "[Setup] cancelled" << std::endl;
      setState(State::Cancelled);
      return;
    }
  std::cerr << "[Setup] interrupted: " << m_failure.toStdString() << std::endl;
  setState(State::Failed, describe(m_failure), m_failure);
}

QString Setup::describe(const QString &failure) const
{
  if (failure.contains(QLatin1String("OTA channel")))
    {
      // http.retrieve: -2 for a network error, else the HTTP status
      if (failure.endsWith(QLatin1String("error: -2")))
        return tr("No connection to the download server. Check the network connection and try again.");
      return tr("The download channel offers no Android for this phone.");
    }
  if (failure.contains(QLatin1String("URLError")) || failure.contains(QLatin1String("urlopen error")) ||
      failure.contains(QLatin1String("timed out")) || failure.contains(QLatin1String("Connection")) ||
      failure.contains(QLatin1String("IncompleteRead")))
    return tr("The download failed. Check the network connection and try again.");
  if (failure.contains(QLatin1String("hash doesn't match")))
    return tr("The download was damaged. Try again.");
  if (failure.contains(QLatin1String("No space left")))
    return tr("There is not enough free space.");
  if (failure.contains(QLatin1String("No images found")))
    return tr("The download channel lists no Android images.");
  return tr("Setting up Android failed.");
}

void Setup::setState(State state, const QString &error, const QString &detail)
{
  if (state == m_state && error == m_error && detail == m_error_detail)
    return;
  m_state = state;
  m_error = error;
  m_error_detail = detail;
  emit stateChanged();
}

void Setup::setStep(const QString &step, double progress, const QString &detail)
{
  m_step = step;
  m_progress = progress;
  m_detail = detail;
  emit progressChanged();
}
