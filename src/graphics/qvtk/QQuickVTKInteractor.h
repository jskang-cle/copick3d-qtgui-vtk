#ifndef Q_QUICK_VTK_INTERACTOR_H
#define Q_QUICK_VTK_INTERACTOR_H

#include <QtCore/QObject>
#include <QQuickWindow>

#include <vtkCommand.h>
#include <vtkRenderWindowInteractor.h>


class QQuickVTKInteractorInternal;

class QQuickVTKInteractor : public vtkRenderWindowInteractor
{
public:
  static QQuickVTKInteractor* New();
  vtkTypeMacro(QQuickVTKInteractor, vtkRenderWindowInteractor);

  /**
   * Enum for additional event types supported.
   * These events can be picked up by command observers on the interactor.
   */
  enum vtkCustomEvents
  {
    ContextMenuEvent = vtkCommand::UserEvent + 100,
    DragEnterEvent,
    DragMoveEvent,
    DragLeaveEvent,
    DropEvent
  };

  /**
   * Overloaded terminate app, which does nothing in Qt.
   * Use qApp->exit() instead.
   */
  void TerminateApp() override;

  /**
   * Overloaded start method does nothing.
   * Use qApp->exec() instead.
   */
  void Start() override;
  void Initialize() override;

  /**
   * Start listening events on 3DConnexion device.
   */
  virtual void StartListening();

  /**
   * Stop listening events on 3DConnexion device.
   */
  virtual void StopListening();

  /**
   * timer event slot
   */
  virtual void TimerEvent(int timerId);

  void SetQtQuickWindow(QQuickWindow* w);

protected:
  // constructor
  QQuickVTKInteractor();
  // destructor
  ~QQuickVTKInteractor() override;

  // create a Qt Timer
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  // destroy a Qt Timer
  int InternalDestroyTimer(int platformTimerId) override;

private:
  QQuickVTKInteractorInternal* Internal;

  QQuickVTKInteractor(const QQuickVTKInteractor&) = delete;
  void operator=(const QQuickVTKInteractor&) = delete;
};

#endif // Q_QUICK_VTK_INTERACTOR_H