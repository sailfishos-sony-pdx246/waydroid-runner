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

#ifndef ANDROID_H
#define ANDROID_H

#include <QObject>
#include <QString>

class QDBusPendingCallWatcher;

// Settings > Apps > Waydroid: whether Android is set up, and its removal.
// Waydroid's root service removes it (id.waydro.Initializer.Remove, a patch
// of this port's Waydroid): it stops Android, deletes Android's images and
// the container's files, and this user's Android data and the launchers of
// Android's apps. The Waydroid app then offers Android's setup again.
class Android : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool setUp READ setUp NOTIFY filesChanged)
  Q_PROPERTY(qint64 bytesUsed READ bytesUsed NOTIFY filesChanged)
  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QString errorDetail READ errorDetail NOTIFY stateChanged)

public:
  enum class State
  {
    Idle = 0,
    Removing = 1,
    Removed = 2,
    Failed = 3,
  };
  Q_ENUM(State)

  explicit Android(QObject *parent = nullptr);

  bool setUp() const { return m_set_up; }
  // Android's images and an unfinished setup's downloads. Not Android's
  // data: it belongs to Android's users and cannot be read from here.
  qint64 bytesUsed() const { return m_bytes_used; }
  State state() const { return m_state; }
  QString error() const { return m_error; }
  QString errorDetail() const { return m_error_detail; }

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void remove();

signals:
  void filesChanged();
  void stateChanged();

private slots:
  void onRemoveReply(QDBusPendingCallWatcher *watcher);

private:
  void setState(State state, const QString &error = QString(), const QString &detail = QString());

  bool    m_set_up{false};
  qint64  m_bytes_used{0};
  State   m_state{State::Idle};
  QString m_error;
  QString m_error_detail;
};

#endif // ANDROID_H
