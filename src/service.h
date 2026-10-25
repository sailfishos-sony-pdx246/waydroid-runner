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

#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>

class QQuickView;
class Runner;

// D-Bus interface of the running instance. The launcher (X-Maemo-Method),
// a second start of the program and scripts reach the instance through it.
// Closing the window does not end Android; the instance quits once the
// session has ended and the window is closed.
//
// "Open" is not QWindow's visibility: Lipstick hides the main window of an
// application with a cover whenever it is not on screen (in the app switcher,
// display off). The window is closed only by a close event (Lipstick's
// close, Hide()) and open again once it is shown.
class Service : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.sailfishosopen.WaydroidRunner")

  Q_PROPERTY(QString State READ state)
  Q_PROPERTY(bool Open READ open)

public:
  static const QString name;
  static const QString path;

  Service(QQuickView *view, Runner *runner, QObject *parent=nullptr);

  QString state() const;
  bool open() const { return m_open; }

  // Ask the instance that owns the D-Bus name to show its window
  static bool activateInstance();

public slots:
  // Launcher entry point: starts Android if it does not run and shows the
  // window. The arguments are the URIs of the launch request (unused).
  Q_SCRIPTABLE void activateWindow(const QStringList &args);
  Q_SCRIPTABLE void Show();
  // As if the user closed the window: Android keeps running
  Q_SCRIPTABLE void Hide();
  Q_SCRIPTABLE void Stop();

signals:
  Q_SCRIPTABLE void StateChanged(const QString &state);
  Q_SCRIPTABLE void OpenChanged(bool open);

protected:
  bool eventFilter(QObject *object, QEvent *event) override;
  void onStatusChanged();
  void onSessionFinished();

protected:
  QQuickView *m_view;
  Runner     *m_runner;
  QString     m_state;
  bool        m_open{false};
};

#endif // SERVICE_H
