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

#ifndef SETUP_H
#define SETUP_H

#include <QObject>
#include <QString>

class QDBusPendingCallWatcher;

// Sets Android up the first time. Waydroid's root service (waydroid
// container) runs its own setup, waydroid init, for any user through D-Bus:
// id.waydro.Initializer on the system bus. It downloads the system and
// vendor images from the channels named in the call, checks their SHA-256
// and unpacks them. It asks for no authorization when the channels are those
// of /usr/share/waydroid-extra/channels.cfg, so the call passes exactly
// these.
class Setup : public QObject
{
  Q_OBJECT

  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QString errorDetail READ errorDetail NOTIFY stateChanged)
  Q_PROPERTY(QString step READ step NOTIFY progressChanged)
  Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(QString detail READ detail NOTIFY progressChanged)
  Q_PROPERTY(qint64 bytesFree READ bytesFree NOTIFY spaceChanged)
  Q_PROPERTY(qint64 bytesNeeded READ bytesNeeded CONSTANT)

public:
  enum class State
  {
    Idle = 0,       // not started yet
    Running = 1,
    Failed = 2,
    Cancelled = 3,
    Done = 4,
  };
  Q_ENUM(State)

  explicit Setup(QObject *parent = nullptr);

  // Waydroid's own test (is_initialized in tools/actions/initializer.py)
  static bool initialized();

  Q_INVOKABLE void start();
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void updateSpace();

  State state() const { return m_state; }
  QString error() const { return m_error; }
  QString errorDetail() const { return m_error_detail; }
  QString step() const { return m_step; }
  // of the current download, 0 to 1; -1 while nothing is downloaded
  double progress() const { return m_progress; }
  QString detail() const { return m_detail; }
  // free space where Waydroid keeps the images; -1 if unknown
  qint64 bytesFree() const { return m_bytes_free; }
  qint64 bytesNeeded() const;

signals:
  void stateChanged();
  void progressChanged();
  void spaceChanged();
  // Android is set up: the session can start
  void finished();

private slots:
  void onProgress(const QString &message);
  void onFinished();
  void onInterrupted();
  void onInitReply(QDBusPendingCallWatcher *watcher);

private:
  void setState(State state, const QString &error = QString(), const QString &detail = QString());
  void setStep(const QString &step, double progress = -1, const QString &detail = QString());
  QString describe(const QString &failure) const;

  State   m_state{State::Idle};
  QString m_error;
  QString m_error_detail;
  QString m_step;
  double  m_progress{-1};
  QString m_detail;
  // the last line of the traceback that Waydroid reported
  QString m_failure;
  qint64  m_bytes_free{-1};
  bool    m_cancel_requested{false};
  bool    m_signals_connected{false};
};

#endif // SETUP_H
