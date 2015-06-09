#!/bin/sh
g++ \
  -stdlib=libstdc++ \
   -undefined dynamic_lookup \
  -F/usr/local/Cellar/qt/4.8.6/lib/ \
  -framework QtCore \
  -framework QtGui \
  -framework QtOpenGL \
  -I/usr/local/Cellar/pyside/1.2.2/include/PySide/ \
  -I/usr/local/Cellar/shiboken/1.2.2/include/shiboken/ \
  -I/Applications/Xcode-5.0.2.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7/ \
  -I/usr/local/Cellar/pyside/1.2.2/include/PySide/QtCore/ \
  -I/usr/local/Cellar/pyside/1.2.2/include/PySide/QtGui/ \
  -I/usr/local/Cellar/pyside/1.2.2/include/PySide/QtOpenGL/ \
  -I/usr/local/include/QtCore \
  -I/usr/local/Cellar/qt/4.8.6/lib/QtGui.framework/Versions/4/Headers/ \
  -IDFG/ \
  -IGraphView/ \
  -I. \
  -I../Services/ \
  -I../../stage/Darwin/x86_64/Debug/include/ \
  -L/usr/local/Cellar/pyside/1.2.2/lib \
  -L/usr/local/Cellar/shiboken/1.2.2/lib \
  -lpyside-python2.7 \
  -lshiboken-python2.7 \
  -L../../stage/Darwin/x86_64/Debug/lib \
  -lFabricUI \
  -lFabricServices \
  -lFabricSplitSearch \
  -lFabricCore \
  -DFEC_SHARED -shared -o FabricUI.so out/FabricUI/*.cpp