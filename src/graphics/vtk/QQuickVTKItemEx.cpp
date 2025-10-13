// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) John Stone
// SPDX-License-Identifier: BSD-3-Clause
#include "QQuickVTKItemEx.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRenderNode>
#include <QtQuick/QSGRendererInterface>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtCore/QThread>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/QThread>
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#include <QtGui/QWheelEvent>
#endif
#endif

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextureObject.h"

#include "QQuickVTKInteractorAdapter.h"
#include "QQuickVTKPinchEvent.h"
#include "QVTKInteractor.h"
#include "QVTKRenderWindowAdapter.h"

// #include "QQuickVTKInteractor.h"

#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include <vtkLogger.h>

// The Qt macro Q_D(X) creates a local variable named 'd' which shadows a private member variable in
// QQmlParserStatus
QT_WARNING_DISABLE_GCC("-Wshadow")
QT_WARNING_DISABLE_CLANG("-Wshadow")
QT_WARNING_DISABLE_MSVC(4458)

// no touch events for now
#define NO_TOUCH

//-------------------------------------------------------------------------------------------------

void QQuickVTKItemEx::setGraphicsApi()
{
  QSurfaceFormat fmt = QVTKRenderWindowAdapter::defaultFormat(false);
  // By default QtQuick sets the alpha buffer size to 0. We follow the same thing here to prevent a
  // transparent background.
  fmt.setAlphaBufferSize(0);
  QSurfaceFormat::setDefaultFormat(fmt);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
#else
  QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
#endif
}

//-------------------------------------------------------------------------------------------------

class QSGVtkObjectNode;

class QQuickVTKItemExPrivate
{
public:
  QQuickVTKItemExPrivate(QQuickVTKItemEx* ptr)
    : q_ptr(ptr)
  {
  }

  QQueue<std::function<void(vtkRenderWindow*, QQuickVTKItemEx::vtkUserData)>> asyncDispatch;

  QQuickVTKInteractorAdapter qt2vtkInteractorAdapter;
  bool scheduleRender = false;

  mutable QSGVtkObjectNode* node = nullptr;

private:
  Q_DISABLE_COPY(QQuickVTKItemExPrivate)
  Q_DECLARE_PUBLIC(QQuickVTKItemEx)
  QQuickVTKItemEx* const q_ptr;
};

namespace
{
bool checkGraphicsApi(QQuickWindow* window)
{
  auto api = window->rendererInterface()->graphicsApi();
  if (api != QSGRendererInterface::OpenGL
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    && api != QSGRendererInterface::OpenGLRhi
#endif
  )
  {
    qFatal(R"***(Error: QtQuick scenegraph is using an unsupported graphics API: %d.
Set the QSG_INFO environment variable to get more information.
Use QQuickVTKItemEx::setupGraphicsApi() to set the OpenGLRhi backend.)***",
      api);
  }
  return true;
}
}
//-------------------------------------------------------------------------------------------------

QQuickVTKItemEx::QQuickVTKItemEx(QQuickItem* parent)
  : QQuickItem(parent)
  , _d_ptr(new QQuickVTKItemExPrivate(this))
{
  setAcceptHoverEvents(true);
#ifndef NO_TOUCH
  setAcceptTouchEvents(true);
#endif
  setAcceptedMouseButtons(Qt::AllButtons);

  setFlag(QQuickItem::ItemIsFocusScope);
  setFlag(QQuickItem::ItemHasContents);
}

QQuickVTKItemEx::~QQuickVTKItemEx() = default;

void QQuickVTKItemEx::dispatch_async(std::function<void(vtkRenderWindow*, vtkUserData)> f)
{
  Q_D(QQuickVTKItemEx);

  d->asyncDispatch.append(f);

  update();
}

class vtkTimerCallback : public vtkCallbackCommand
{
public:
  vtkTimerCallback() = default;

  static vtkTimerCallback* New();
  vtkTypeMacro(vtkTimerCallback, vtkCallbackCommand);

  virtual void Execute(vtkObject* caller, unsigned long eventId,
                       void* vtkNotUsed(callData))
  {
    if (vtkCommand::TimerEvent == eventId && Item)
    {
      QMetaObject::invokeMethod(Item, [=]() { Item->update(); }, Qt::QueuedConnection);
    }
  }

public:
  QQuickVTKItemEx* Item = nullptr;
};

vtkStandardNewMacro(vtkTimerCallback);

class QSGVtkObjectNode
  : public QSGTextureProvider
  , public QSGSimpleTextureNode
{
  Q_OBJECT
public:
  QSGVtkObjectNode() { qsgnode_set_description(this, QStringLiteral("vtknode")); }

  ~QSGVtkObjectNode() override
  {
    if (m_item)
      m_item->destroyingVTK(vtkWindow, vtkUserData);

    delete QSGVtkObjectNode::texture();

    // Cleanup the VTK window resources
    vtkWindow->GetRenderers()->InitTraversal();
    while (auto renderer = vtkWindow->GetRenderers()->GetNextItem())
      renderer->ReleaseGraphicsResources(vtkWindow);
    vtkWindow->ReleaseGraphicsResources(vtkWindow);
    vtkWindow = nullptr;

    // Cleanup the User Data
    vtkUserData = nullptr;
  }

  QSGTexture* texture() const override { return QSGSimpleTextureNode::texture(); }

  void initialize(QQuickVTKItemEx* item)
  {
    // Create and initialize the vtkWindow
    vtkWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWindow->SetMultiSamples(0);
    vtkWindow->SetReadyForRendering(false);
    vtkWindow->SetFrameBlitModeToNoBlit();
    auto loadFunc = [](void*, const char* name) -> vtkOpenGLRenderWindow::VTKOpenGLAPIProc
    {
      if (auto context = QOpenGLContext::currentContext())
      {
        if (auto* symbol = context->getProcAddress(name))
        {
          return symbol;
        }
      }
      return nullptr;
    };
    vtkWindow->SetOpenGLSymbolLoader(loadFunc, nullptr);
    vtkNew<QVTKInteractor> iren;
    iren->SetRenderWindow(vtkWindow);
    // iren->SetQtQuickWindow(item->window());
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    iren->SetInteractorStyle(style);
    vtkUserData = item->initializeVTK(vtkWindow);
    auto* ia = vtkWindow->GetInteractor();
    if (ia && !QVTKInteractor::SafeDownCast(ia))
    {
      qWarning().nospace() << "QQuickVTKItemEx.cpp:" << __LINE__
                           << ", Only QQuickVTKInteractor is supported";
      return;
    }
    vtkWindow->SetReadyForRendering(false);
    vtkWindow->GetInteractor()->Initialize();
    vtkWindow->SetMapped(true);
    vtkWindow->SetIsCurrent(true);
    vtkWindow->SetForceMaximumHardwareLineWidth(1);
    vtkWindow->SetOwnContext(false);
    vtkWindow->OpenGLInitContext();

    timerCallback = vtkSmartPointer<vtkTimerCallback>::New();
    timerCallback->Item = item;
    ia->AddObserver(vtkCommand::TimerEvent, timerCallback);
  }

  void scheduleRender()
  {
    // Update only if we have a window and a render is not already queued.
    if (m_window && !m_renderPending)
    {
      m_renderPending = true;
      m_window->update();
    }
  }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void render()
  {
    if (m_renderPending)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const bool needsWrap = m_window &&
        QSGRendererInterface::isApiRhiBased(m_window->rendererInterface()->graphicsApi());
      if (needsWrap)
        m_window->beginExternalCommands();
#endif

      // Render VTK into it's framebuffer
      auto ostate = vtkWindow->GetState();
      ostate->Reset();
      ostate->Push();
      ostate->vtkglDepthFunc(GL_LEQUAL); // note: By default, Qt sets the depth function to GL_LESS
      // but VTK expects GL_LEQUAL
      vtkWindow->SetReadyForRendering(true);
      vtkWindow->GetInteractor()->ProcessEvents();
      vtkWindow->GetInteractor()->Render();
      vtkWindow->SetReadyForRendering(false);
      ostate->Pop();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      if (needsWrap)
        m_window->endExternalCommands();
#endif

      m_renderPending = false;
      markDirty(QSGNode::DirtyMaterial);
      Q_EMIT textureChanged();
    }
  }

  void handleScreenChange(QScreen*)
  {
    if (!m_window || !m_item)
      return;

    if (m_window->effectiveDevicePixelRatio() != m_devicePixelRatio)
    {

      m_item->update();
    }
  }

private:
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> vtkWindow;
  vtkSmartPointer<vtkObject> vtkUserData;
  vtkSmartPointer<vtkTimerCallback> timerCallback;
  bool m_renderPending = false;

protected:
  // variables set in QQuickVTKItemEx::updatePaintNode()
  QPointer<QQuickWindow> m_window;
  QPointer<QQuickVTKItemEx> m_item;
  qreal m_devicePixelRatio = 0;
  QSizeF m_size;
  friend class QQuickVTKItemEx;
};

QSGNode* QQuickVTKItemEx::updatePaintNode(QSGNode* node, UpdatePaintNodeData*)
{
  auto* n = static_cast<QSGVtkObjectNode*>(node);

  // Don't create the node if our size is invalid
  if (!n && (width() <= 0 || height() <= 0))
    return nullptr;

  Q_D(QQuickVTKItemEx);

  // Create the QSGRenderNode
  if (!n)
  {
    if (!checkGraphicsApi(window()))
      return nullptr;
    if (!d->node)
      d->node = new QSGVtkObjectNode;
    n = d->node;
  }

  // Initialize the QSGRenderNode
  if (!n->m_item)
  {
    n->initialize(this);
    n->m_window = window();
    n->m_item = this;
    connect(window(), &QQuickWindow::beforeRendering, n, &QSGVtkObjectNode::render);
    connect(window(), &QQuickWindow::screenChanged, n, &QSGVtkObjectNode::handleScreenChange);
  }

  // Watch for size changes
  auto size = QSizeF(width(), height());
  n->m_devicePixelRatio = window()->devicePixelRatio();
  d->qt2vtkInteractorAdapter.SetDevicePixelRatio(n->m_devicePixelRatio);
  auto sz = size * n->m_devicePixelRatio;
  bool dirtySize = sz != n->m_size;
  if (dirtySize)
  {
    n->vtkWindow->SetSize(sz.width(), sz.height());
    n->vtkWindow->GetInteractor()->SetSize(n->vtkWindow->GetSize());
    delete n->texture();
    n->m_size = sz;
  }

  // Dispatch commands to VTK
  if (!d->asyncDispatch.empty())
  {
    n->scheduleRender();

    n->vtkWindow->SetReadyForRendering(true);
    while (!d->asyncDispatch.empty())
      d->asyncDispatch.dequeue()(n->vtkWindow, n->vtkUserData);
    n->vtkWindow->SetReadyForRendering(false);
  }

  // Whenever the size changes we need to get a new FBO from VTK so we need to render right now
  // (with the gui-thread blocked) for this one frame.
  if (dirtySize)
  {
    n->scheduleRender();
    n->render();
    auto fb = n->vtkWindow->GetDisplayFramebuffer();
    if (fb && fb->GetNumberOfColorAttachments() > 0)
    {
      GLuint texId = fb->GetColorAttachmentAsTextureObject(0)->GetHandle();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      auto* texture =
        window()->createTextureFromId(texId, sz.toSize(), QQuickWindow::TextureIsOpaque);
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      auto* texture = window()->createTextureFromNativeObject(
        QQuickWindow::NativeObjectTexture, &texId, 0, sz.toSize(), QQuickWindow::TextureIsOpaque);
#else
      auto* texture = QNativeInterface::QSGOpenGLTexture::fromNative(
        texId, window(), sz.toSize(), QQuickWindow::TextureIsOpaque);
#endif
      n->setTexture(texture);
    }
    else if (!fb)
      qFatal("%s %d %s", "QQuickVTKItemEx.cpp:", __LINE__,
        ", YIKES!!, Render() didn't create a FrameBuffer!?");
    else
      qFatal("%s %d %s", "QQuickVTKItemEx.cpp:", __LINE__,
        ", YIKES!!, Render() didn't create any ColorBufferAttachements in its FrameBuffer!?");
  }

  n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
  n->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
  n->setRect(0, 0, width(), height());

  if (d->scheduleRender)
  {
    n->scheduleRender();
    d->scheduleRender = false;
  }

  return n;
}

void QQuickVTKItemEx::scheduleRender()
{
  Q_D(QQuickVTKItemEx);

  d->scheduleRender = true;
  update();
}

bool QQuickVTKItemEx::isTextureProvider() const
{
  return true;
}

QSGTextureProvider* QQuickVTKItemEx::textureProvider() const
{
  // When Item::layer::enabled == true, QQuickItem will be a texture provider.
  // In this case we should prefer to return the layer rather than the VTK texture.
  if (QQuickItem::isTextureProvider())
    return QQuickItem::textureProvider();

  if (!checkGraphicsApi(window()))
    return nullptr;

  Q_D(const QQuickVTKItemEx);

  if (!d->node)
    d->node = new QSGVtkObjectNode;

  return d->node;
}

void QQuickVTKItemEx::releaseResources()
{
  // When release resources is called on the GUI thread, we only need to
  // forget about the node. Since it is the node we returned from updatePaintNode
  // it will be managed by the scene graph.
  Q_D(QQuickVTKItemEx);
  d->node = nullptr;
}

void QQuickVTKItemEx::invalidateSceneGraph()
{
  Q_D(QQuickVTKItemEx);
  d->node = nullptr;
}

bool QQuickVTKItemEx::event(QEvent* ev)
{
  Q_D(QQuickVTKItemEx);

  if (!ev)
    return false;

  auto e = ev->clone();
  dispatch_async(
    [d, e](vtkRenderWindow* vtkWindow, vtkUserData) mutable
    {
      d->qt2vtkInteractorAdapter.ProcessEvent(e, vtkWindow->GetInteractor());
      delete e;
    });

  ev->accept();

  return true;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKItemEx::pinchHandlerRotate(const QPointF& position, double delta)
{
  Q_D(QQuickVTKItemEx);
  auto c = QSharedPointer<QQuickVTKPinchEvent>::create(QQuickVTKPinchEvent::QQuickVTKPinch,
    QQuickVTKPinchEvent::QQUICKVTK_ROTATE, position, QVector2D(0, 0), 1.0, delta);
  dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable
    { d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor()); });
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKItemEx::pinchHandlerScale(const QPointF& position, double delta)
{
  Q_D(QQuickVTKItemEx);
  auto c = QSharedPointer<QQuickVTKPinchEvent>::create(QQuickVTKPinchEvent::QQuickVTKPinch,
    QQuickVTKPinchEvent::QQUICKVTK_SCALE, position, QVector2D(0, 0), delta);
  dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable
    { d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor()); });
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKItemEx::pinchHandlerTranslate(const QPointF& position, const QVector2D& delta)
{
  Q_D(QQuickVTKItemEx);
  auto c = QSharedPointer<QQuickVTKPinchEvent>::create(
    QQuickVTKPinchEvent::QQuickVTKPinch, QQuickVTKPinchEvent::QQUICKVTK_TRANSLATE, position, delta);
  dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable
    { d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor()); });
}

#include "QQuickVTKItemEx.moc"