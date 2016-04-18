//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <FabricUI/GraphView/Port.h>
#include <FabricUI/GraphView/PortLabel.h>

#include <QtGui/QApplication>
#include <QtGui/QGraphicsSceneMouseEvent>

namespace FabricUI {
namespace GraphView {

PortLabel::PortLabel(
  QGraphicsWidget * parent,
  QString const &text,
  QColor color,
  QColor hlColor,
  QFont font
  )
  : TextContainer(
    parent,
    text,
    color,
    hlColor,
    font
    )
{
}

void PortLabel::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
  if ( !!(event->buttons() & Qt::LeftButton) )
  {
    m_dragStartPosition = event->pos();
    event->accept();
  }
  else TextContainer::mousePressEvent( event );
}

void PortLabel::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
{
  if ( !!(event->buttons() & Qt::LeftButton) )
  {
    if ( (event->pos() - m_dragStartPosition).manhattanLength()
       >= QApplication::startDragDistance() )
    {
      event->accept();

      Port *port = static_cast<Port *>( parentItem() );

      QDrag *drag = new QDrag( event->widget() );

      Port::MimeData *mimeData = new Port::MimeData( port );
      drag->setMimeData( mimeData );

      Qt::DropAction dropAction = drag->exec( Qt::MoveAction );
      (void)dropAction;

      return;
    }
  }

  TextContainer::mouseMoveEvent( event );
}

void PortLabel::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
  if ( !!(event->buttons() & Qt::LeftButton) )
  {
    event->accept();
  }
  else TextContainer::mouseReleaseEvent( event );  
}

} // namespace GraphView
} // namespace FabricUI
