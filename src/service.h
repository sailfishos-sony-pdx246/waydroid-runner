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

class MDConfItem;
class QQuickView;
class Runner;
class Setup;

// D-Bus interface of the running instance. The launchers (X-Maemo-Method:
// the Waydroid icon and the Android apps' icons), a tap on an Android
// notification (Lipstick's remote action), a second start of the program and
// scripts reach the instance through it.
// Closing the window keeps Android running, unless the setting closeActionKey
// (Settings > Apps > Waydroid) is "stop"; the instance quits once the session
// has ended and the window is closed. While Android is being set up (Setup),
// closing the window lets the setup finish; the instance quits after it.
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
  // dconf key: what closing the window does, "keep-running" (also when
  // unset) or "stop"
  static const QString closeActionKey;

  Service(QQuickView *view, Runner *runner, Setup *setup, QObject *parent=nullptr);

  QString state() const;
  bool open() const { return m_open; }

  // Ask the instance that owns the D-Bus name to show its window
  static bool activateInstance();

public slots:
  // Launcher entry point. The Waydroid icon calls it without arguments:
  // Show(). An Android app's launcher passes the app's package first
  // (X-Maemo-Fixed-Args): Launch(package).
  Q_SCRIPTABLE void activateWindow(const QStringList &args);
  // Open an Android app in the window, and show it
  Q_SCRIPTABLE void Launch(const QString &package);
  // Show the window; starts Android if it does not run
  Q_SCRIPTABLE void Show();
  // As if the user closed the window: Android keeps running, unless the
  // setting is "stop"
  Q_SCRIPTABLE void Hide();
  Q_SCRIPTABLE void Stop();

signals:
  Q_SCRIPTABLE void StateChanged(const QString &state);
  Q_SCRIPTABLE void OpenChanged(bool open);

protected:
  bool eventFilter(QObject *object, QEvent *event) override;
  void onStatusChanged();
  void onSessionFinished();
  void onCloseActionChanged();
  bool stopOnClose() const;
  void onSetupStateChanged();
  void onSetupFinished();

protected:
  QQuickView *m_view;
  Runner     *m_runner;
  Setup      *m_setup;
  MDConfItem *m_close_action;
  QString     m_state;
  bool        m_open{false};
};

#endif // SERVICE_H
