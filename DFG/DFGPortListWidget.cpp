// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#include "DFGPortListWidget.h"
#include "DFGPortListItem.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>

using namespace FabricServices;
using namespace FabricUI;
using namespace FabricUI::DFG;

DFGPortListWidget::DFGPortListWidget(QWidget * parent, DFGController * controller, const DFGConfig & config)
: QWidget(parent)
{
  m_controller = controller;
  m_config = config;
  m_exec = NULL;

  setMinimumHeight(80);
  setMaximumHeight(80);
  setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);
  setContentsMargins(0, 0, 0, 0);
  layout->setContentsMargins(0, 0, 0, 0);

  m_list = new QListWidget(this);
  layout->addWidget(m_list);
}

DFGPortListWidget::~DFGPortListWidget()
{
  if(m_exec)
    delete(m_exec);
}

void DFGPortListWidget::setExec(DFGWrapper::Executable exec)
{
  if(m_exec)
    delete(m_exec);
  m_exec = new DFGWrapper::Executable(exec);

  m_list->clear();

  try
  {
    std::vector<DFGWrapper::Port> ports = exec.getPorts();
    for(size_t i=0;i<ports.size();i++)
    {
      QString portType = " in";
      if(ports[i].getPortType() == FabricCore::DFGPortType_Out)
        portType = "out";
      else if(ports[i].getPortType() == FabricCore::DFGPortType_IO)
        portType = " io";
      QString dataType = ports[i].getDataType().c_str();
      QString name = ports[i].getName().c_str();
      DFGPortListItem * item = new DFGPortListItem(m_list, portType, dataType, name);
      m_list->addItem(item);
    }
  }
  catch(FabricCore::Exception e)
  {
    m_controller->logError(e.getDesc_cstr());
  }
}

QString DFGPortListWidget::selectedItem() const
{
  DFGPortListItem * item = (DFGPortListItem*)m_list->currentItem();
  if(item)
    return item->name();
  return "";
}
