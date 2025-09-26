// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable : 4127)
#pragma warning(disable : 4512)
#endif

#include "QQuickVTKInteractor.h"

#include <QEvent>
#include <QResizeEvent>
#include <QSignalMapper>
#include <QTimer>

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

struct QQuickVTKInteractorInternal : public QObject
{
    Q_OBJECT
public:
    QQuickVTKInteractorInternal(QQuickVTKInteractor* p);
    ~QQuickVTKInteractorInternal() override;
public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void TimerEvent(int id);

public: // NOLINT(readability-redundant-access-specifiers)
  QSignalMapper* SignalMapper;
  typedef std::map<int, QTimer*> TimerMap;
  TimerMap Timers;
  QQuickVTKInteractor* Parent;
  QQuickWindow* QtQuickWindow;
};

QQuickVTKInteractorInternal::QQuickVTKInteractorInternal(QQuickVTKInteractor* p)
  : Parent(p)
{
  this->SignalMapper = new QSignalMapper(this);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  QObject::connect(
    this->SignalMapper, &QSignalMapper::mappedInt, this, &QQuickVTKInteractorInternal::TimerEvent);
#else
  QObject::connect(this->SignalMapper, SIGNAL(mapped(int)), this, SLOT(TimerEvent(int)));
#endif
}

QQuickVTKInteractorInternal::~QQuickVTKInteractorInternal() = default;

void QQuickVTKInteractorInternal::TimerEvent(int id)
{
  Parent->TimerEvent(id);
}

/*! allocation method for Qt/VTK interactor
 */
vtkStandardNewMacro(QQuickVTKInteractor);

/*! constructor for Qt/VTK interactor
 */
QQuickVTKInteractor::QQuickVTKInteractor()
{
  this->Internal = new QQuickVTKInteractorInternal(this);
}

void QQuickVTKInteractor::Initialize()
{
  this->Initialized = 1;
  this->Enable();
}

/*! start method for interactor
 */
void QQuickVTKInteractor::Start()
{
  vtkErrorMacro(<< "QQuickVTKInteractor cannot control the event loop.");
}

/*! terminate the application
 */
void QQuickVTKInteractor::TerminateApp()
{
  // we are in a GUI so let's terminate the GUI the normal way
  // qApp->exit();
}

//------------------------------------------------------------------------------
void QQuickVTKInteractor::StartListening()
{
}

//------------------------------------------------------------------------------
void QQuickVTKInteractor::StopListening()
{
}

/*! handle timer event
 */
void QQuickVTKInteractor::TimerEvent(int timerId)
{
  if (!this->GetEnabled())
  {
    return;
  }
  this->InvokeEvent(vtkCommand::TimerEvent, (void*)&timerId);

  if (this->IsOneShotTimer(timerId))
  {
    this->DestroyTimer(timerId); // 'cause our Qt timers are always repeating
  }
}

/*! constructor
 */
QQuickVTKInteractor::~QQuickVTKInteractor()
{
  delete this->Internal;
}

/*! create Qt timer with an interval of 10 msec.
 */
int QQuickVTKInteractor::InternalCreateTimer(
  int timerId, int vtkNotUsed(timerType), unsigned long duration)
{
  QTimer* timer = new QTimer(this->Internal);
  timer->start(duration);
  this->Internal->SignalMapper->setMapping(timer, timerId);
  QObject::connect(timer, SIGNAL(timeout()), this->Internal->SignalMapper, SLOT(map()));
  int platformTimerId = timer->timerId();
  this->Internal->Timers.insert(
    QQuickVTKInteractorInternal::TimerMap::value_type(platformTimerId, timer));
  return platformTimerId;
}

/*! destroy timer
 */
int QQuickVTKInteractor::InternalDestroyTimer(int platformTimerId)
{
  QQuickVTKInteractorInternal::TimerMap::iterator iter = this->Internal->Timers.find(platformTimerId);
  if (iter != this->Internal->Timers.end())
  {
    iter->second->stop();
    iter->second->deleteLater();
    this->Internal->Timers.erase(iter);
    return 1;
  }
  return 0;
}

void QQuickVTKInteractor::SetQtQuickWindow(QQuickWindow* w)
{
  this->Internal->QtQuickWindow = w;
}

VTK_ABI_NAMESPACE_END

#include "QQuickVTKInteractor.moc"
