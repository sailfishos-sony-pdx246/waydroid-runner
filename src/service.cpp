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

#include "service.h"
#include "runner.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QEvent>
#include <QGuiApplication>
#include <QMetaObject>
#include <QQuickItem>
#include <QQuickView>

#include <MDConfItem>

#include <iostream>

const QString Service::name = QStringLiteral("org.sailfishosopen.WaydroidRunner");
const QString Service::path = QStringLiteral("/org/sailfishosopen/WaydroidRunner");
const QString Service::closeActionKey = QStringLiteral("/apps/waydroid-runner/close_action");

Service::Service(QQuickView *view, Runner *runner, QObject *parent):
  QObject(parent),
  m_view(view),
  m_runner(runner)
{
  m_state = state();
  connect(m_runner, &Runner::statusChanged, this, &Service::onStatusChanged);
  connect(m_runner, &Runner::exit, this, &Service::onSessionFinished);
  m_view->installEventFilter(this);

  m_close_action = new MDConfItem(closeActionKey, this);
  connect(m_close_action, &MDConfItem::valueChanged, this, &Service::onCloseActionChanged);
}

QString Service::state() const
{
  switch (m_runner->statusCode())
    {
    case Runner::Status::Idle:
      return QStringLiteral("stopped");
    case Runner::Status::StartingSession:
      return QStringLiteral("starting");
    case Runner::Status::WaitingForUI:
      return QStringLiteral("running");
    case Runner::Status::StoppingSession:
      return QStringLiteral("stopping");
    default:
      return QStringLiteral("error");
    }
}

bool Service::activateInstance()
{
  QDBusMessage call = QDBusMessage::createMethodCall(name, path, name, QStringLiteral("activateWindow"));
  call << QStringList();
  QDBusMessage reply = QDBusConnection::sessionBus().call(call);
  if (reply.type() == QDBusMessage::ErrorMessage)
    {
      std::cerr << "Cannot reach the running instance: " << reply.errorMessage().toStdString() << "\n";
      return false;
    }
  std::cerr << "Showed the running instance\n";
  return true;
}

void Service::activateWindow(const QStringList &args)
{
  if (!args.isEmpty() && !args.first().isEmpty())
    Launch(args.first());
  else
    Show();
}

void Service::Launch(const QString &package)
{
  m_runner->launchApp(package);
  Show();
}

void Service::Show()
{
  // Android has stopped or ended since the instance started: start it again
  if (!m_runner->sessionActive())
    m_runner->start();

  // Silica's ApplicationWindow shows the window full screen and raises it
  QQuickItem *window = m_view->rootObject();
  if (!window || !QMetaObject::invokeMethod(window, "activate"))
    m_view->showFullScreen();
}

void Service::Hide()
{
  m_view->close();
}

void Service::Stop()
{
  m_runner->stopSession();
}

void Service::onStatusChanged()
{
  QString s = state();
  if (s != m_state)
    {
      m_state = s;
      emit StateChanged(m_state);
    }
}

bool Service::eventFilter(QObject *object, QEvent *event)
{
  if (object == m_view && event->type() == QEvent::Show && !m_open)
    {
      m_open = true;
      emit OpenChanged(m_open);
    }
  else if (object == m_view && event->type() == QEvent::Close && m_open)
    {
      m_open = false;
      emit OpenChanged(m_open);

      // Closed while Android does not run: nothing to keep
      if (!m_runner->sessionActive())
        qApp->quit();
      else if (stopOnClose())
        m_runner->stopSession();
    }
  return QObject::eventFilter(object, event);
}

void Service::onSessionFinished()
{
  // Nothing left to run in the background
  if (!m_open)
    qApp->quit();
}

bool Service::stopOnClose() const
{
  return m_close_action->value().toString() == QLatin1String("stop");
}

void Service::onCloseActionChanged()
{
  // Set to "stop" while Android runs with the window closed: as if it had
  // been closed now. Not at the start: a hidden start (-prestart) has not
  // been closed.
  if (stopOnClose() && !m_open && m_runner->sessionActive())
    m_runner->stopSession();
}
