// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

#include <FTL/Str.h>
#include <FTL/MapCharSingle.h>

#include "DFGController.h"
#include "DFGLogWidget.h"
#include "Commands/DFGAddNodeCommand.h"
#include "Commands/DFGAddEmptyGraphCommand.h"
#include "Commands/DFGAddEmptyFuncCommand.h"
#include "Commands/DFGRemoveNodeCommand.h"
#include "Commands/DFGRenameNodeCommand.h"
#include "Commands/DFGAddConnectionCommand.h"
#include "Commands/DFGRemoveConnectionCommand.h"
#include "Commands/DFGAddPortCommand.h"
#include "Commands/DFGRemovePortCommand.h"
#include "Commands/DFGRenamePortCommand.h"
#include "Commands/DFGSetCodeCommand.h"
#include "Commands/DFGSetArgCommand.h"
#include "Commands/DFGSetDefaultValueCommand.h"
#include "Commands/DFGSetNodeCacheRuleCommand.h"

using namespace FabricServices;
using namespace FabricUI;
using namespace FabricUI::DFG;

DFGController::DFGController(GraphView::Graph * graph, FabricServices::Commands::CommandStack * stack, FabricCore::Client * client, DFGWrapper::Host * host, bool overTakeBindingNotifications)
: GraphView::Controller(graph, stack)
, m_client(client)
, m_host(host)
{
  m_view = NULL;
  m_host = host;
  m_logFunc = NULL;
  m_overTakeBindingNotifications = overTakeBindingNotifications;
  m_presetDictsUpToDate = false;

  QObject::connect(this, SIGNAL(argsChanged()), this, SLOT(checkErrors()));
}

FabricCore::Client * DFGController::getClient()
{
  return m_client;
}

void DFGController::setClient(FabricCore::Client * client)
{
  m_client = client;
}

DFGWrapper::Host * DFGController::getHost()
{
  return m_host;
}

DFGWrapper::Binding DFGController::getBinding()
{
  return getView()->getGraph().getWrappedCoreBinding();
}

void DFGController::setHost(FabricServices::DFGWrapper::Host * host)
{
  m_host = host;
}

DFGView * DFGController::getView()
{
  return m_view;
}

void DFGController::setView(DFGView * view)
{
  if(m_view && m_overTakeBindingNotifications)
    getBinding().setNotificationCallback(NULL, NULL);

  m_view = view;
  if(m_view)
  {
    m_view->setController(this);
    try
    {
      m_view->onGraphSet();
    }
    catch(FabricCore::Exception e)
    {
      logError(e.getDesc_cstr());
    }

    if(m_overTakeBindingNotifications)
      getBinding().setNotificationCallback(bindingNotificationCallback, this);
  }
}

QString DFGController::addNodeFromPreset(QString path, QString preset, QPointF pos)
{
  try
  {
    DFGAddNodeCommand * command = new DFGAddNodeCommand(this, path, preset, pos);
    if(!addCommand(command))
    {
      delete(command);
      return "";
    }
    emit structureChanged();
    emit recompiled();
    return command->getNodePath().c_str();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return "";
}

QString DFGController::addEmptyGraph(QString path, QString title, QPointF pos)
{
  try
  {
    DFGAddEmptyGraphCommand * command = new DFGAddEmptyGraphCommand(this, path, title, pos);
    if(!addCommand(command))
    {
      delete(command);
      return "";
    }
    emit structureChanged();
    emit recompiled();
    return command->getNodePath().c_str();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return "";
}

QString DFGController::addEmptyFunc(QString path, QString title, QPointF pos)
{
  try
  {
    DFGAddEmptyFuncCommand * command = new DFGAddEmptyFuncCommand(this, path, title, pos);
    if(!addCommand(command))
    {
      delete(command);
      return "";
    }

    std::string path = command->getNodePath();

    std::string code;
    code += "dfgEntry {\n";
    code += "  // result = a + b;\n";
    code += "}\n";

    DFGSetCodeCommand * setCodeCommand = new DFGSetCodeCommand(this, path.c_str(), code.c_str());
    if(!addCommand(setCodeCommand))
    {
      delete(setCodeCommand);
      return path.c_str();
    }

    emit structureChanged();
    emit recompiled();
    return path.c_str();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }

  return "";
}

bool DFGController::removeNode(QString path)
{
  try
  {
    DFGRemoveNodeCommand * removeCommand = new DFGRemoveNodeCommand(this, path);
    if(!addCommand(removeCommand))
    {
      delete(removeCommand);
      return false;
    }
    emit structureChanged();
    emit recompiled();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

bool DFGController::removeNode(GraphView::Node * node)
{
  return removeNode(node->path());
}

bool DFGController::renameNode(QString path, QString title)
{
  try
  {
    DFGRenameNodeCommand * command = new DFGRenameNodeCommand(this, path, title);
    if(!addCommand(command))
    {
      delete(command);
      return false;
    }
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

bool DFGController::renameNode(GraphView::Node * node, QString title)
{
  if(node->title() == title)
    return false;
  return renameNode(node->path(), title);
}

GraphView::Pin * DFGController::addPin(GraphView::Node * node, QString name, GraphView::PortType pType, QColor color, QString dataType)
{
  // disabled, pins are created by the DFGView
  return NULL;
}

bool DFGController::removePin(GraphView::Pin * pin)
{
  // disabled, pins are created by the DFGView
  return false;
}

QString DFGController::addPort(QString path, QString name, FabricCore::DFGPortType pType, QString dataType)
{
  GraphView::PortType portType = GraphView::PortType_Input;
  if(pType == FabricCore::DFGPortType_In)
    portType = GraphView::PortType_Output;
  else if(pType == FabricCore::DFGPortType_IO)
    portType = GraphView::PortType_IO;
  return addPort(path, name, portType, dataType);
}

QString DFGController::addPort(QString path, QString name, GraphView::PortType pType, QString dataType)
{
  QString result;
  try
  {
    DFGAddPortCommand * command = new DFGAddPortCommand(this, path, name, pType, dataType);
    if(!addCommand(command))
      delete(command);
    else
      result = command->getPortPath();

    // if this port is on the binding graph
    DFGWrapper::GraphExecutable exec = getView()->getGraph();
    DFGWrapper::Binding binding = exec.getWrappedCoreBinding();
    if(binding.getGraph().getPath() == path.toUtf8().constData())
    {
      if(dataType[0] != '$')
      {
        DFGSetArgCommand * argCommand = new DFGSetArgCommand(this, command->getPortName(), dataType);
        if(!addCommand(argCommand))
          delete(argCommand);
        m_view->updateDataTypesOnPorts();
      }        
    }

    emit structureChanged();
    emit recompiled();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return "";
  }
  return result;
}

bool DFGController::removePort(QString path, QString name)
{
  try
  {
    DFGRemovePortCommand * command = new DFGRemovePortCommand(this, path, name);
    if(!addCommand(command))
      delete(command);
    emit argsChanged();
    emit structureChanged();
    emit recompiled();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

GraphView::Port * DFGController::addPortFromPin(GraphView::Pin * pin, GraphView::PortType pType)
{
  if(!graph())
    return NULL;

  beginInteraction();
  QString portPath = addPort(graph()->path(), pin->name(), pType, pin->dataType());
  QString portName = GraphView::lastPathSegment(portPath);
  if(portPath.length() > 0)
  {
    std::vector<GraphView::Connection*> connections = graph()->connections();
    if(pType == GraphView::PortType_Output)
    {
      for(size_t i=0;i<connections.size();i++)
      {
        if(connections[i]->dst() == pin)
        {
          if(!removeConnection(connections[i]->src(), connections[i]->dst()))
          {
            endInteraction();
            return NULL;
          }
          break;
        }
      }
      addConnection(portPath, pin->path(), false, true);
    }
    else if(pType == GraphView::PortType_Input)
    {
      for(size_t i=0;i<connections.size();i++)
      {
        if(connections[i]->dst()->targetType() == GraphView::TargetType_Port)
        {
          if(((GraphView::Port*)connections[i]->dst())->path() == portPath)
          {
            if(!removeConnection(connections[i]->src(), connections[i]->dst()))
            {
              endInteraction();
              return NULL;
            }
            break;
          }
        }
      }
      addConnection(pin->path(), portPath, true, false);
    }
  }
  endInteraction();
  return graph()->port(portName);
}

QString DFGController::renamePort(QString path, QString name)
{
  if(GraphView::lastPathSegment(path) == name)
    return "";
  try
  {
    DFGRenamePortCommand * command = new DFGRenamePortCommand(this, path, name);
    if(!addCommand(command))
    {
      delete(command);
      return "";
    }
    emit argsChanged();
    emit structureChanged();
    emit recompiled();

    QString newName = command->getResult();
    emit portRenamed(path, newName);
    return newName;
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return "";
  }
  return "";
}

bool DFGController::addConnection(QString srcPath, QString dstPath, bool srcIsPin, bool dstIsPin)
{
  beginInteraction();
  try
  {
    DFGWrapper::Port port = getPortFromPath(dstPath.toUtf8().constData());
    if(port.isValid())
    {
      std::vector<std::string> sources = port.getSources();
      if(sources.size() > 0)
      {
        Commands::Command * command = new DFGRemoveConnectionCommand(this, 
          sources[0].c_str(), 
          dstPath.toUtf8().constData(),
          sources[0].find('.') != std::string::npos,
          dstIsPin
        );
        if(!addCommand(command))
        {
          delete(command);
          return false;
        }
      }
    }
    Commands::Command * command = new DFGAddConnectionCommand(this, 
      srcPath.toUtf8().constData(), 
      dstPath.toUtf8().constData(),
      srcIsPin,
      dstIsPin
    );
    if(addCommand(command))
    {
      bindUnboundRTVals();
      endInteraction();
      emit argsChanged();
      emit structureChanged();
      emit recompiled();
      return true;
    }
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  endInteraction();
  return false;
}

bool DFGController::addConnection(GraphView::ConnectionTarget * src, GraphView::ConnectionTarget * dst)
{
  QString srcPath;
  if(src->targetType() == GraphView::TargetType_Pin)
    srcPath = ((GraphView::Pin*)src)->path();
  else if(src->targetType() == GraphView::TargetType_Port)
    srcPath = ((GraphView::Port*)src)->path();
  QString dstPath;
  if(dst->targetType() == GraphView::TargetType_Pin)
    dstPath = ((GraphView::Pin*)dst)->path();
  else if(dst->targetType() == GraphView::TargetType_Port)
    dstPath = ((GraphView::Port*)dst)->path();
  return addConnection(srcPath, dstPath,
    src->targetType() == GraphView::TargetType_Pin,
    dst->targetType() == GraphView::TargetType_Pin
  );
}

bool DFGController::removeConnection(QString srcPath, QString dstPath, bool srcIsPin, bool dstIsPin)
{
  // filter out IO ports
  if(!srcIsPin && !dstIsPin)
  {
    if(srcPath == dstPath)
      return false;
  }

  try
  {
    Commands::Command * command = new DFGRemoveConnectionCommand(this, 
      srcPath.toUtf8().constData(), 
      dstPath.toUtf8().constData(),
      srcIsPin,
      dstIsPin
    );

    if(addCommand(command))
    {
      emit argsChanged();
      emit structureChanged();
      emit recompiled();
      return true;
    }
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

bool DFGController::removeConnection(GraphView::ConnectionTarget * src, GraphView::ConnectionTarget * dst)
{
  QString srcPath;
  if(src->targetType() == GraphView::TargetType_Pin)
    srcPath = ((GraphView::Pin*)src)->path();
  else if(src->targetType() == GraphView::TargetType_Port)
    srcPath = ((GraphView::Port*)src)->path();
  QString dstPath;
  if(dst->targetType() == GraphView::TargetType_Pin)
    dstPath = ((GraphView::Pin*)dst)->path();
  else if(dst->targetType() == GraphView::TargetType_Port)
    dstPath = ((GraphView::Port*)dst)->path();
  return removeConnection(srcPath, dstPath,
    src->targetType() == GraphView::TargetType_Pin,
    dst->targetType() == GraphView::TargetType_Pin
  );
}

bool DFGController::addExtensionDependency(QString extension, QString execPath)
{
  try
  {
    std::string execPathStr = execPath.toUtf8().constData();
    DFGWrapper::Executable exec = m_view->getGraph();
    if(exec.getPath() != execPathStr)
    {
      execPathStr = GraphView::relativePathSTL(exec.getPath(), execPathStr);

      while(execPathStr.length() > 0)
      {
        std::string nodeName = execPathStr;
        if(nodeName.find('.') != std::string::npos)
        {
          nodeName = nodeName.substr(0, execPathStr.find('.'));
          execPathStr = execPathStr.substr(nodeName.length()+1, execPathStr.length());
        }
        else
          execPathStr = "";

        if(exec.getObjectType() != "Graph")
          return false;

        DFGWrapper::GraphExecutable graph = exec;
        DFGWrapper::Node node = graph.getNode(nodeName.c_str());
        exec = node.getExecutable();
      }
    }
    exec.addExtensionDependency(extension.toUtf8().constData());
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

bool DFGController::setCode(QString path, QString code)
{
  try
  {
    Commands::Command * command = new DFGSetCodeCommand(this, path, code);;
    if(addCommand(command))
    {
      emit recompiled();
      return true;
    }
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

bool DFGController::setArg(QString argName, QString dataType, QString json)
{
  beginInteraction();
  try
  {
    Commands::Command * command = new DFGSetArgCommand(this, argName, dataType, json);;
    if(addCommand(command))
    {
      // check other ports which have no value bound and set an arg on them too
      DFGWrapper::GraphExecutable exec = m_view->getGraph();
      FabricCore::DFGBinding binding = exec.getWrappedCoreBinding();
      std::vector<DFGWrapper::Port> ports = exec.getPorts();

      bindUnboundRTVals();
      emit argsChanged();
      endInteraction();
      return true;
    }
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  endInteraction();
  return false;
}

bool DFGController::setArg(QString argName, FabricCore::RTVal value)
{
  try
  {
    Commands::Command * command = new DFGSetArgCommand(this, argName, value);;
    if(addCommand(command))
    {
      emit argsChanged();
      return true;
    }
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

bool DFGController::setDefaultValue(QString path, FabricCore::RTVal value)
{
  try
  {
    Commands::Command * command = new DFGSetDefaultValueCommand(this, path, value);;
    if(addCommand(command))
    {
      emit argsChanged();
      return true;
    }
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

bool DFGController::setDefaultValue(QString path, QString dataType, QString json)
{
  try
  {
    FabricCore::RTVal value = FabricCore::ConstructRTValFromJSON(*getClient(), dataType.toUtf8().constData(), json.toUtf8().constData());
    return setDefaultValue(path, value);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

QString DFGController::exportJSON(QString path)
{
  try
  {
    DFGWrapper::Executable exec = getExecFromPath(path.toUtf8().constData());
    return exec.exportJSON().c_str();
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return "";
}

bool DFGController::setNodeCacheRule(QString path, FEC_DFGCacheRule rule)
{
  try
  {
    DFGWrapper::Node node = getNodeFromPath(path.toUtf8().constData());
    if(node.getCacheRule() == rule)
      return false;
    Commands::Command * command = new DFGSetNodeCacheRuleCommand(this, path, rule);;
    if(addCommand(command))
      return true;
    else
      delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

bool DFGController::moveNode(QString path, QPointF pos, bool isTopLeftPos)
{
  try
  {
    DFGWrapper::Node node = getNodeFromPath(path.toUtf8().constData());
    QString metaData = "{\"x\": "+QString::number(pos.x())+", \"y\": "+QString::number(pos.y())+"}";
    node.setMetadata("uiGraphPos", metaData.toUtf8().constData(), false);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

bool DFGController::moveNode(GraphView::Node * uiNode, QPointF pos, bool isTopLeftPos)
{
  return moveNode(uiNode->path(), pos, isTopLeftPos);
}

bool DFGController::zoomCanvas(float zoom)
{
  try
  {
    DFGWrapper::GraphExecutable exec = m_view->getGraph();
    QString metaData = "{\"value\": "+QString::number(zoom)+"}";
    exec.setMetadata("uiGraphZoom", metaData.toUtf8().constData(), false);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

bool DFGController::panCanvas(QPointF pan)
{
  try
  {
    DFGWrapper::GraphExecutable exec = m_view->getGraph();
    QString metaData = "{\"x\": "+QString::number(pan.x())+", \"y\": "+QString::number(pan.y())+"}";
    exec.setMetadata("uiGraphPan", metaData.toUtf8().constData(), false);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
    return false;
  }
  return true;
}

void DFGController::checkErrors()
{
  DFGWrapper::GraphExecutable exec = m_view->getGraph();
  std::vector<DFGWrapper::Node> nodes = exec.getNodes();

  for(size_t j=0;j<nodes.size();j++)
  {
    GraphView::Node * uiNode = NULL; 
    if(graph())
    {
      std::string path = GraphView::relativePathSTL(m_view->getGraph().getPath(), nodes[j].getPath().c_str());
      uiNode = graph()->nodeFromPath(path.c_str());
      if(!uiNode)
        continue;
      uiNode->clearError();
    }

    std::vector<std::string> errors = nodes[j].getExecutable().getErrors();
    std::string errorComposed;
    for(size_t i=0;i<errors.size();i++)
    {
      if(errorComposed.length() > 0)
        errorComposed += "\n";
      errorComposed += errors[i];
    }

    if(errorComposed.length() > 0 && uiNode)
    {
      logError((nodes[j].getPath() + " : " +errorComposed).c_str());
      uiNode->setError(errorComposed.c_str());
    }
  }
}

void DFGController::log(const char * message)
{
  DFGLogWidget::callback(NULL, message, 0);
  if(m_logFunc)
    (*m_logFunc)(message);
}

void DFGController::logError(const char * message)
{
  std::string m = "Error: ";
  m += message;
  log(m.c_str());
}

void DFGController::setLogFunc(LogFunc func)
{
  m_logFunc = func;
}


bool DFGController::execute()
{
  DFGWrapper::GraphExecutable exec = m_view->getGraph();
  try
  {
    exec.getWrappedCoreBinding().execute();
    return true;
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

void DFGController::onValueChanged(ValueEditor::ValueItem * item)
{
  beginInteraction();
  try
  {
    std::string itemPath = item->path().toUtf8().constData();
    DFGWrapper::Executable exec = m_view->getGraph();
    std::string portOrPinPath = GraphView::relativePathSTL(exec.getPath(), itemPath);

    // let's assume it is pin, if there's still a node name in it
    Commands::Command * command = NULL;
    if(portOrPinPath.find('.') != std::string::npos)
    {
      command = new DFGSetDefaultValueCommand(this, itemPath.c_str(), item->value());
    }
    else
    {
      command = new DFGSetArgCommand(this, item->name().toUtf8().constData(), item->value());
    }
    if(addCommand(command))
      return;
    delete(command);
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  endInteraction();
}

bool DFGController::bindUnboundRTVals(std::string dataType)
{
  try
  {
    DFGWrapper::GraphExecutable exec = m_view->getGraph();
    FabricCore::DFGBinding binding = exec.getWrappedCoreBinding();
    std::vector<DFGWrapper::Port> ports = exec.getPorts();

    bool argsHaveChanged = false;

    for(size_t i=0;i<ports.size();i++)
    {
      std::string dataTypeToCheck = dataType;
      if(dataTypeToCheck.length() == 0)
        dataTypeToCheck = ports[i].getDataType();
      if(dataTypeToCheck.length() == 0)
        continue;
      else if(ports[i].getDataType() != dataTypeToCheck)
        continue;

      // if there is already a bound value, make sure it has the right type
      FabricCore::RTVal value;
      try
      {
        value = binding.getArgValue(ports[i].getName().c_str());
      }
      catch(FabricCore::Exception e)
      {
        continue;
      }
      if(value.isValid())
      {
        if(value.getTypeName().getStringCString() == dataTypeToCheck)
          continue;
      }

      addCommand(new DFGSetArgCommand(this, ports[i].getName().c_str(), dataTypeToCheck.c_str()));
      argsHaveChanged = true;
    }
    return argsHaveChanged;
  }
  catch(FabricCore::Exception e)
  {
    logError(e.getDesc_cstr());
  }
  return false;
}

bool DFGController::canConnect( QString pathA, QString pathB, QString &failureDesc )
{
  std::string failureDescStdString;
  bool result = getBinding().canConnect(
    pathA.toUtf8().constData(),
    pathB.toUtf8().constData(),
    failureDescStdString
    );
  if ( !result )
    failureDesc = failureDescStdString.c_str();
  return result;
}

void DFGController::populateNodeToolbar(GraphView::NodeToolbar * toolbar, GraphView::Node * node)
{
  Controller::populateNodeToolbar(toolbar, node);
  toolbar->addTool("node_edit", "node_edit.png");
}

DFGWrapper::Node DFGController::getNodeFromPath(const std::string & path)
{
  DFGWrapper::GraphExecutable graph = getBinding().getGraph();
  std::string relPath = GraphView::relativePathSTL(graph.getPath(), path);
  std::string nodeName = relPath;
  int period = relPath.rfind('.');
  if(period != std::string::npos)
  {
    graph = getGraphExecFromPath(path.substr(0, period));
    nodeName = path.substr(period+1, relPath.length());
  }
  else
    graph = getBinding().getGraph();

  return graph.getNode(nodeName.c_str());
}

DFGWrapper::Executable DFGController::getExecFromPath(const std::string & path)
{
  DFGWrapper::GraphExecutable graph = getBinding().getGraph();
  std::string relPath = GraphView::relativePathSTL(graph.getPath(), path);
  if(relPath.length() == 0)
    return graph;
  std::string nodeName = relPath;
  int period = relPath.rfind('.');
  if(period != std::string::npos)
  {
    graph = getGraphExecFromPath(path.substr(0, period));
    nodeName = path.substr(period+1, relPath.length());
  }
  else
    graph = getBinding().getGraph();

  return graph.getNode(nodeName.c_str()).getExecutable();
}

DFGWrapper::GraphExecutable DFGController::getGraphExecFromPath(const std::string & path)
{
  DFGWrapper::GraphExecutable graph = getBinding().getGraph();
  std::string relPath = GraphView::relativePathSTL(graph.getPath(), path);
  while(relPath.length() > 0)
  {
    std::string graphName = relPath;
    int period = relPath.find('.');
    if(period != std::string::npos)
    {
      graph = getGraphExecFromPath(path.substr(0, period));
      graphName = path.substr(0, period);
      relPath = relPath.substr(period+1, relPath.length());
    }
    else
      relPath = "";

    graph = graph.getNode(graphName.c_str()).getExecutable();
  }
  return graph;
}

FabricServices::DFGWrapper::Port DFGController::getPortFromPath(const std::string & path)
{
  std::string parent = GraphView::parentPathSTL(path);
  std::string portName = GraphView::lastPathSegmentSTL(path);

  DFGWrapper::Executable exec = getExecFromPath(parent);
  return exec.getPort(portName.c_str());
}

void DFGController::nodeToolTriggered(FabricUI::GraphView::Node * node, QString toolName)
{
  Controller::nodeToolTriggered(node, toolName);

  if(toolName == "node_edit")
  {
    emit nodeEditRequested(node);
  }
}

void DFGController::bindingNotificationCallback(void * userData, char const *jsonCString, uint32_t jsonLength)
{
  if(!jsonCString)
    return;
  DFGController * ctrl = (DFGController *)userData;

  FabricCore::Variant notificationsVar = FabricCore::Variant::CreateFromJSON(jsonCString, jsonLength);
  for(uint32_t i=0;i<notificationsVar.getArraySize();i++)
  {
    const FabricCore::Variant * notificationVar = notificationsVar.getArrayElement(i);
    const FabricCore::Variant * descVar = notificationVar->getDictValue("desc");
    std::string descStr = descVar->getStringData();

    // if(descStr == "argTypeChanged")
    // {
    //   ctrl->m_view->updateDataTypesOnPorts();
    // }
  }

}

QStringList DFGController::getPresetPathsFromSearch(QString search, bool includePresets, bool includeNameSpaces)
{
  if(search.length() == 0)
    return QStringList();

  updatePresetPathDB();

  QStringList results;

  // [pzion 20150305] This is a little evil but avoids lots of copying

  std::string stdString = search.toUtf8().constData();

  std::vector<FTL::StrRef> searchSplit;
  FTL::StrSplit<'.'>( stdString, searchSplit, true /*strict*/ );

  FTL::StrRemap< FTL::MapCharSingle<'.', '\0'> >( stdString );

  char const **cStrs = reinterpret_cast<char const **>(
    alloca( searchSplit.size() * sizeof( char const * ) )
    );
  for ( size_t i = 0; i < searchSplit.size(); ++i )
    cStrs[i] = searchSplit[i].data();

  if(includePresets)
  {
    SplitSearch::Matches matches =
      m_presetPathDict.search( searchSplit.size(), cStrs );

    std::vector<const char *> userDatas;
    userDatas.resize(matches.getSize());
    matches.getUserdatas(matches.getSize(), (const void**)&userDatas[0]);

    for(size_t i=0;i<userDatas.size();i++)
      results.push_back(userDatas[i]);
  }
  if(includeNameSpaces)
  {
    SplitSearch::Matches matches = m_presetNameSpaceDict.search( searchSplit.size(), cStrs );

    std::vector<const char *> userDatas;
    userDatas.resize(matches.getSize());
    matches.getUserdatas(matches.getSize(), (const void**)&userDatas[0]);

    for(size_t i=0;i<userDatas.size();i++)
      results.push_back(userDatas[i]);
  }

  return results;
}

void DFGController::updatePresetPathDB()
{
  if(m_presetDictsUpToDate)
    return;
  m_presetDictsUpToDate = true;

  m_presetNameSpaceDict.clear();
  m_presetPathDict.clear();
  m_presetNameSpaceDictSTL.clear();
  m_presetPathDictSTL.clear();

  std::vector<DFGWrapper::NameSpace> namespaces;
  namespaces.push_back(m_host->getRootNameSpace());

  for(size_t i=0;i<namespaces.size();i++)
  {
    m_presetNameSpaceDictSTL.push_back(namespaces[i].getPath());

    std::vector<DFGWrapper::NameSpace> childNameSpaces = namespaces[i].getNameSpaces();
    namespaces.insert(namespaces.end(), childNameSpaces.begin(), childNameSpaces.end());

    std::vector<DFGWrapper::Object> presets = namespaces[i].getPresets();
    for(size_t j=0;j<presets.size();j++)
      m_presetPathDictSTL.push_back(presets[j].getPath());
  }

  for(size_t i=0;i<m_presetNameSpaceDictSTL.size();i++)
    m_presetNameSpaceDict.add(m_presetNameSpaceDictSTL[i].c_str(), '.', m_presetNameSpaceDictSTL[i].c_str());
  for(size_t i=0;i<m_presetPathDictSTL.size();i++)
    m_presetPathDict.add(m_presetPathDictSTL[i].c_str(), '.', m_presetPathDictSTL[i].c_str());
}
