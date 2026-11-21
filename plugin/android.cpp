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

#include "android.h"
#include "androidstate.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace {

const QString waydroidService = QStringLiteral("id.waydro.Container");
const QString initializerPath = QStringLiteral("/Initializer");
const QString initializerInterface = QStringLiteral("id.waydro.Initializer");

// The Waydroid app's running instance (src/service.h)
const QString runnerService = QStringLiteral("org.sailfishosopen.WaydroidRunner");
const QString runnerPath = QStringLiteral("/org/sailfishosopen/WaydroidRunner");

// Waydroid replies when all is removed; many apps' data take a while
const int removeTimeout = 10 * 60 * 1000;

qint64 filesSize(const QString &dir)
{
  qint64 size = 0;
  for (const QFileInfo &f: QDir(dir).entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks))
    size += f.size();
  return size;
}

}

Android::Android(QObject *parent):
  QObject(parent)
{
  refresh();
}

void Android::refresh()
{
  bool set_up = AndroidState::isSetUp();
  qint64 used = filesSize(AndroidState::workDir() + QStringLiteral("/images")) +
      filesSize(AndroidState::workDir() + QStringLiteral("/cache_http"));
  if (set_up != m_set_up || used != m_bytes_used)
    {
      m_set_up = set_up;
      m_bytes_used = used;
      emit filesChanged();
    }
}

void Android::remove()
{
  if (m_state == State::Removing)
    return;

  setState(State::Removing);

  // The Waydroid app, if it runs: close its window, so that it quits once
  // Waydroid has ended its session. It is not started for this.
  QDBusConnection session = QDBusConnection::sessionBus();
  if (session.interface() && session.interface()->isServiceRegistered(runnerService))
    {
      QDBusMessage hide = QDBusMessage::createMethodCall(runnerService, runnerPath, runnerService,
                                                         QStringLiteral("Hide"));
      hide.setAutoStartService(false);
      session.asyncCall(hide);
    }

  QDBusMessage call = QDBusMessage::createMethodCall(waydroidService, initializerPath, initializerInterface,
                                                     QStringLiteral("Remove"));
  QDBusPendingCallWatcher *watcher =
      new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(call, removeTimeout), this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this, &Android::onRemoveReply);
}

void Android::onRemoveReply(QDBusPendingCallWatcher *watcher)
{
  QDBusPendingReply<> reply = *watcher;
  watcher->deleteLater();
  refresh();

  if (!reply.isError())
    {
      setState(State::Removed);
      return;
    }

  QDBusError e = reply.error();
  qWarning() << "Removing Android failed:" << e.name() << e.message();
  // D-Bus activation starts Waydroid's service through systemd; when that
  // start fails, the bus only times out the activation
  if (e.type() == QDBusError::ServiceUnknown)
    setState(State::Failed, tr("Waydroid's service is not available."), e.message());
  else if (e.type() == QDBusError::TimedOut || e.name().startsWith(QLatin1String("org.freedesktop.DBus.Error.Spawn")))
    setState(State::Failed, tr("Waydroid's service did not start."), e.message());
  else if (e.type() == QDBusError::NoReply || e.type() == QDBusError::Timeout ||
           e.type() == QDBusError::Disconnected)
    setState(State::Failed, tr("Waydroid did not finish removing Android."), e.message());
  else if (e.type() == QDBusError::UnknownMethod)
    setState(State::Failed, tr("This version of Waydroid cannot remove Android."), e.message());
  else
    setState(State::Failed, tr("Android could not be removed."), e.message());
}

void Android::setState(State state, const QString &error, const QString &detail)
{
  if (state == m_state && error == m_error && detail == m_error_detail)
    return;
  m_state = state;
  m_error = error;
  m_error_detail = detail;
  emit stateChanged();
}
