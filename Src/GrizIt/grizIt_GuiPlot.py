#!/usr/local/bin/python
#  !/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
############################################################################
# grizIt_Plot.py - GrizIt Plot class based on MatPlotLib
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      July 30, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - July 30, 2010: Created.
#
############################################################################
#
""" 
Series of data are loaded from a .csv file, and their names are
displayed in a checkable list view. The user can select the series
it wants from the list and plot them on a matplotlib canvas.

"""

import PySide
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

from PySide.QtCore import *
from PySide.QtGui import *
import PySide.QtXml

import os, sys, re, bisect, math, copy, time, csv
sys.path.append("/usr/local/lib/python2.6/site-packages")
sys.path.append("/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/lib/python2.6/site-packages")

os.environ['QT_API'] = 'pyside'

import numpy as np
from numpy import arange, sin, pi

import matplotlib as mpl
# mpl.use('Qt4Agg')
import matplotlib.pyplot as plt

# mpl.rcParams['backend.qt4']='PySide'

from matplotlib.figure import Figure
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as FigureCanvas

from matplotlib.font_manager import FontProperties
from matplotlib import *

import grizIt_Session
import grizIt_GuiMain as GUI
# import grizIt_GuiMiscFunctions as MISC

session = grizIt_Session.session
env     = grizIt_Session.env

command=""
timer = QtCore.QTimer()

# fontLegend=MISC.make_font("serif", 7, "normal", "normal", "green")

################
# Misc Commands
################

# This Class fixes a problem with the Nav Toolbar not being
# able to load files ending in .svg. We just rename them
# to png and the icons will load properly


#################################################
# This class manages data for a set of TH curves
#################################################
class DataManager(object):
    def __init__(self, filename=None):
        self.xdata = {}
        self.ydata = {}
        self.names = []
        self.checked = {}
        self.lw = {}
        self.marker = {}
        self.color = {}
        self.series_min={}
        self.series_max={}
        
     #end def
    
    def load_from_file(self, filename=None):
        self.xdata = {}
        self.ydata = {}
        self.names = []
        self.tempData = {}
        self.index=0
        if filename:
            for line in csv.reader(open(filename, 'rb')):
                self.names.append(line[0])
                self.temp_data = map(int, line[1:])
                self.datalen = len(line[1:])
                for i in range(len(self.tempData)):
                    self.xdata[name].append(self.tempData[self.index])
                    self.ydata[name].append(self.tempData[self.index+1])
                    self.index+=2
                        
    def add_curve(self, name, xdata, ydata, lw, color):
        self.xdata[name] = xdata
        self.ydata[name] = ydata
        self.names.append(name)
        self.checked[name]=False
        
        self.lw[name]=lw
        self.color[name]=color 
        self.series_names()
        self.get_minmax(name)
                           
     #end def

    def delete_curve(self, name ):
        index = self.names.index(name)
        del self.names[index]
    #end def
    
    def series_names(self):
        """ Names of the data series
        """
        return self.names
    #end def
    
    def series_count(self):
        return len(self.names)
    #end def
    
    def get_minmax(self, name):
        self.series_min[name] = min(self.xdata[name])
        self.series_max[name] = max(self.xdata[name])
    #end def
    
    def get_series_data(self, name, x_from, x_to):
        x_data=[]
        y_data=[]
        y_index=0
        for i in self.xdata[name]:
            if (i>=x_from and i<=x_to):
                 x_data.append(i)
                 y_data.append(self.ydata[name][y_index])
                 y_index=y_index+1

        return x_data, y_data
    #end def
    
                
class PlotWin(QMainWindow):
    def __init__(self, parent=None):
        super(PlotWin, self).__init__(parent)
        self.setWindowTitle('GrizIt TimeHistory Plot')
        self.plotTitle=""
        self.xTitle=""
        self.yTitle=""
        
        self.series = DataManager()
        self.series_list_model = QStandardItemModel()

        self.create_menu()
        self.create_main_frame()
        self.create_status_bar()
        
        self.update_ui()
        self.on_show()

        self.show()
    #end def
    
    def load_file(self, filename=None):
        filename = QFileDialog.getOpenFileName(self,
            'Open a data file', '.', 'CSV files (*.csv);;All Files (*.*)')
        
        if filename:
            self.series.load_from_file(filename)
            self.fill_series_list(self.series.series_names())
            self.status_text.setText("Loaded " + filename)
            self.update_ui()
    #end def
        
    def get_checked_minmax(self):
        min=sys.float_info.max
        max=-sys.float_info.min
        num_checked=0
        
        for row in range(self.series_list_model.rowCount()):
            model_index = self.series_list_model.index(row, 0)
            checked = self.series_list_model.data(model_index,
                 Qt.CheckStateRole) == QVariant(Qt.Checked)
            name = str(self.series_list_model.data(model_index).toString())

            if checked:
               num_checked=num_checked+1
               if (self.series.series_min[name]<min):
                   min=self.series.series_min[name]
               if (self.series.series_min[name]>max):
                   max=self.series.series_max[name]
                   
        if (num_checked==0):
            min=0
            max=0

        return min,max
     #end def           
        
    def update_ui(self):
        min, max = self.get_checked_minmax()
        
        if self.series.series_count() > 0:
            self.from_spin.setValue(min)
            self.from_spin.setRange(min,max)
            self.to_spin.setValue(max)
            
            self.from_spin.setEnabled(True)
            self.to_spin.setEnabled(True)
        else:
            self.from_spin.setEnabled(False)
            self.to_spin.setEnabled(False)
    #end def
    
    def update_series(self, names):
        self.series_list_model.clear()
        
        for name in names:
            item = QStandardItem(name)
            item.setCheckState(QtCore.Qt.Checked)
            item.setCheckable(True)
            self.series_list_model.appendRow(item)
    #end def

    def add_curve(self, name, xdata, ydata, lw, color):
        self.series.add_curve(name, xdata, ydata, lw, color)
        self.update_series(self.series.names)
        self.update_ui()
    #end def

    def delete_curve(self, name):
        self.series.delete_curve(name)
        self.update_series(self.series.names)
        self.update_ui()
    #end def
    
    def update_figure(self):
        gcf().canvas.draw()
    #end def
    
    def update_plot_attributes(self):
#        font=MISC.make_font("serif", 10, "normal", "normal", "green")
        self.axes.set_title(self.plotTitle, fontproperties=font)
        font=MISC.make_font("serif", 7, "normal", "normal", "green")
        xtext = self.axes.set_xlabel(self.xTitle, fontproperties=font) # returns a Text instance
        ytext = self.axes.set_ylabel(self.yTitle, fontproperties=font) # returns a Text instance
    #end def

    
    def on_show(self):
        self.axes.clear()        
        has_series = False
        self.update_plot_attributes()
        
        for row in range(self.series_list_model.rowCount()):
            model_index = self.series_list_model.index(row, 0)
            checked = self.series_list_model.data(model_index,
                 Qt.CheckStateRole) == QVariant(Qt.Checked)
            name = str(self.series_list_model.data(model_index).toString())
            lw=self.series.lw[name]
            color=self.series.color[name]

            if checked:
                self.series.checked[name] = True
                has_series = True
                
                x_from = self.from_spin.value()
                x_to   = self.to_spin.value()
                new_lw = self.lw_spin.value()
                if (new_lw>0.0):
                    lw = new_lw
                
                xdata, ydata = self.series.get_series_data(name, x_from, x_to)

                marker=""
                if (self.marker_cb.isChecked()):
                    marker="o"
            
                self.axes.plot(xdata, ydata, 'o-', label=name, linewidth=lw, color=color, marker=marker, markerfacecolor="blue", markersize=2)

        # Enable Legend
        if has_series and self.legend_cb.isChecked():
           # Shink current axis by 20% to make room for the Legend
           self.axes.set_position([self.fig_size.x0, self.fig_size.y0, self.fig_size.width * 0.85, self.fig_size.height])
           self.axes.legend(prop=fontLegend, loc='upper left', bbox_to_anchor=(1, 1), title="Legend")
        else:
           self.axes.set_position([self.fig_size.x0, self.fig_size.y0, self.fig_size.width, self.fig_size.height])
            
        # Enable Grid
        if (has_series and self.grid_cb.isChecked()):
            self.axes.grid(True)
        else:
            self.axes.grid(False)

        # Set axes Scale
        if (has_series and self.logx_cb.isChecked()):
            self.axes.set_xscale("log")
        else:
            self.axes.set_xscale("linear")
        if (has_series and self.logy_cb.isChecked()):
            self.axes.set_yscale("log")
        else:
            self.axes.set_yscale("linear")

        gcf().canvas.draw()
    #end def

    def on_select(self):
        if ( self.select_cb.isChecked() ):
             self.unselect_cb.setChecked(False)
             for row in range(self.series_list_model.rowCount()):
                 item = self.series_list_model.item(row, 0)
                 item.setCheckState(QtCore.Qt.Checked)
             self.on_show()               
    #end def
    def on_unselect(self):
        if ( self.unselect_cb.isChecked() ):
             self.select_cb.setChecked(False)
             for row in range(self.series_list_model.rowCount()):
                 item = self.series_list_model.item(row, 0)
                 item.setCheckState(QtCore.Qt.Unchecked)
             self.on_show()
    #end def
    
    def on_list_clicked(self):
        self.update_ui()
    #end def
    
    def on_about(self):
        msg = __doc__
        QMessageBox.about(self, "About the demo", msg.strip())
    #end def
    
    def create_main_frame(self):
        self.main_frame = QWidget()
        
        plot_frame = QWidget()
        
        self.dpi = 150
        self.fig = Figure(figsize=(8,12), dpi=self.dpi)
        gcf().canvas = FigureCanvas(self.fig)
        gcf().canvas.setParent(self.main_frame)
        
        self.axes = self.fig.add_subplot(111)
        self.fig_size = self.axes.get_position()
        
        self.mpl_toolbar = VMToolbar(gcf().canvas, self.main_frame)
        
        label1 = QLabel("Data series")
        self.series_list_view = QListView()
        self.series_list_view.setModel(self.series_list_model)
        self.series_list_view.clicked.connect(self.on_list_clicked)
        
        spin_label1 = QLabel('X from')
        self.from_spin = QDoubleSpinBox()
        self.from_spin.valueChanged.connect(self.on_show)
        self.from_spin.setSingleStep(1.0)
        self.from_spin.setDecimals(8)
        self.from_spin.setRange(0, 100)
        self.from_spin.setEnabled(True)
        
        spin_label2 = QLabel('to')
        self.to_spin = QDoubleSpinBox()
        self.to_spin.valueChanged.connect(self.on_show)
        self.to_spin.setSingleStep(1.0)
        self.to_spin.setDecimals(8)
        self.to_spin.setRange(0, 100)
        self.to_spin.setEnabled(True)
       
        spin_hbox = QHBoxLayout()
        spin_hbox.addWidget(spin_label1)
        spin_hbox.addWidget(self.from_spin)
        spin_hbox.addWidget(spin_label2)
        spin_hbox.addWidget(self.to_spin)
        spin_hbox.addStretch(1)
        
        spins2_hbox = QHBoxLayout()
        spin_label2 = QLabel('to')
        
        # Show check boxes
        self.legend_cb = QCheckBox("Show L&egend")
        self.legend_cb.setChecked(False)
        self.legend_cb.stateChanged.connect(self.on_show)
        
        self.grid_cb = QCheckBox("Show G&rid")
        self.grid_cb.setChecked(False)
        self.grid_cb.stateChanged.connect(self.on_show)
       
        self.marker_cb = QCheckBox("Show M&arker")
        self.marker_cb.setChecked(False)
        self.marker_cb.stateChanged.connect(self.on_show)

        show_hbox = QHBoxLayout()
        show_hbox.addWidget(self.legend_cb)
        show_hbox.addWidget(self.grid_cb)
        show_hbox.addWidget(self.marker_cb)
        show_hbox.addStretch(1)

        # Log check boxes
        self.logx_cb = QCheckBox("Log Scale X")
        self.logx_cb.setChecked(False)
        self.logx_cb.stateChanged.connect(self.on_show)

        self.logy_cb = QCheckBox("Log Scale Y")
        self.logy_cb.setChecked(False)
        self.logy_cb.stateChanged.connect(self.on_show)

        # Log check boxes
        self.select_cb = QCheckBox("Select All Curves")
        self.select_cb.setChecked(False)
        self.select_cb.stateChanged.connect(self.on_select)
        self.unselect_cb = QCheckBox("Unselect All Curves")
        self.unselect_cb.setChecked(False)
        self.unselect_cb.stateChanged.connect(self.on_unselect)
        self.select_hbox = QHBoxLayout()
        self.select_hbox.setAlignment(Qt.AlignCenter)
        self.select_hbox.addWidget(self.select_cb)
        self.select_hbox.addWidget(self.unselect_cb)
        self.select_hbox.addStretch(0)

        log_hbox = QHBoxLayout()
        log_hbox.setAlignment(Qt.AlignCenter)
        log_hbox.addWidget(self.logx_cb)
        log_hbox.addWidget(self.logy_cb)
        log_hbox.addStretch(0)
        
        show_button = QPushButton("&Show")
        show_button.setFixedSize(show_button.sizeHint())
        show_button.setStyleSheet("background-color: rgb(100, 200,0);\n")
        self.connect(show_button, SIGNAL('clicked()'), self.on_show)
        show_hbox2 = QGridLayout()
        show_hbox2.addWidget(show_button, 0, 0, Qt.AlignCenter)
        
        hboxIncrement = QHBoxLayout()
        incrementLabel = QLabel('X Range Increment:')
        self.incrementInput = QtGui.QLineEdit()
        self.incrementInput.setFixedSize(show_button.sizeHint())
        hboxIncrement.addWidget(incrementLabel)
        hboxIncrement.addWidget(self.incrementInput) 
        self.connect(self.incrementInput, SIGNAL('returnPressed()'),   self.update_increment)
        self.connect(self.incrementInput, SIGNAL('editingFinished()'), self.update_increment)
               
        self.lw_label = QLabel('Line Width (.1-5)')
        self.lw_spin = QDoubleSpinBox()
        self.lw_spin.valueChanged.connect(self.on_show)
        self.lw_spin.setSingleStep(0.1)
        self.lw_spin.setDecimals(3)
        self.lw_spin.setRange(0.0, 5)
        self.lw_spin.setValue(0.0)
        self.lw_spin.setEnabled(True)
        self.lw_hbox = QHBoxLayout()
        self.lw_hbox.addWidget(self.lw_label)
        self.lw_hbox.addWidget(self.lw_spin)
        
        left_vbox = QVBoxLayout()
        left_vbox.addWidget(self.canvas)
        left_vbox.addWidget(self.mpl_toolbar)

        right_vbox1 = QGridLayout()
        right_vbox1.setSpacing(10)
        right_vbox1.addWidget(label1,0,0,Qt.AlignCenter)
        
        right_vbox2 = QGridLayout()
        right_vbox2.addWidget(self.series_list_view,2,0)
        right_vbox2.addLayout(self.select_hbox,3,0)
        right_vbox2.addLayout(hboxIncrement,4,0)
        right_vbox2.addLayout(spin_hbox,5,0)
        right_vbox2.addLayout(show_hbox,6,0)
        right_vbox2.addLayout(log_hbox,7,0,Qt.AlignCenter)
        right_vbox2.addWidget(self.series_list_view,8,0)
        right_vbox2.addLayout(self.lw_hbox,9,0,Qt.AlignCenter)
        right_vbox2.addLayout(show_hbox2,10,0)

        right_vbox = QVBoxLayout()
        right_vbox.addLayout(right_vbox1)
        right_vbox.addLayout(right_vbox2)
        right_vbox.addWidget(self.series_list_view)
        
        splitter = QSplitter(Qt.Horizontal, self)
        hbox = QHBoxLayout()
        hbox.addLayout(left_vbox)
        hbox.addLayout(right_vbox)
        self.main_frame.setLayout(hbox)

        self.setCentralWidget(self.main_frame)
        self.start_timer()
    #end def

    def update_increment(self):
        self.xIncrement = str(self.incrementInput.text())
        increment = float(self.xIncrement)
        self.to_spin.setSingleStep(increment)
        self.from_spin.setSingleStep(increment)
    #end def
    
    def create_status_bar(self):
        self.status_text = QLabel("Please load a data file")
        self.statusBar().addWidget(self.status_text, 1)
    #end def
    
    def create_menu(self):        
        self.file_menu = self.menuBar().addMenu("&File")
        
        load_action = self.create_action("&Load file",
            shortcut="Ctrl+L", slot=self.load_file, tip="Load a file")
        quit_action = self.create_action("&Quit", slot=self.close, 
            shortcut="Ctrl+Q", tip="Close the application")
        
        self.add_actions(self.file_menu, 
            (load_action, None, quit_action))
            
        self.help_menu = self.menuBar().addMenu("&Help")
        about_action = self.create_action("&About", 
            shortcut='F1', slot=self.on_about, 
            tip='About the demo')
        
        self.add_actions(self.help_menu, (about_action,))
    #end def
    
    def add_actions(self, target, actions):
        for action in actions:
            if action is None:
                target.addSeparator()
            else:
                target.addAction(action)
    #end def
    
    def create_action(  self, text, slot=None, shortcut=None, 
                        icon=None, tip=None, checkable=False,
                        signal="triggered()"):
        action = QAction(text, self)
        if icon is not None:
            action.setIcon(QIcon(":/%s.png" % icon))
        if shortcut is not None:
            action.setShortcut(shortcut)
        if tip is not None:
            action.setToolTip(tip)
            action.setStatusTip(tip)
        if slot is not None:
            self.connect(action, SIGNAL(signal), slot)
        if checkable:
            action.setCheckable(True)
        return action
    #end def
    
    def fileQuit(self):
        self.close()
    #end def
    
    def closeEvent(self, ce):
        self.fileQuit()
    #end def
    
    def start_timer(self):
        timer = QtCore.QTimer(self)
        QtCore.QObject.connect(timer, QtCore.SIGNAL("timeout()"), self.update_figure)
        timer.start(1600)
    #end def
    
    def about(self):
        QtGui.QMessageBox.about(self, "About %s" % progname, " ")
    #end def        
    
    def deleteCurve(self, id):
        self.axes.lines.remove(self.lines[id])
        del self.curveNames[id]
    #end def

    def setPlotTitle(self, plotTitle, xTitle, yTitle):
        self.plotTitle = plotTitle
        self.xTitle    = xTitle
        self.yTitle    = yTitle
    #end def

###################
# Testing Driver
###################
if __name__ == "__main__":
    i=0
    foo=""
    qApp    = QtGui.QApplication(sys.argv)
    myplot  = PlotWin()
    myplot.setPlotTitle("Plot Title", "X Axis", "Y Axis")
    
    xdata = [1.0*x+1 for x in range(100)] #That's 1 to 100 in steps of 1
    ydata = [math.sin(x) for x in xdata]    
    id = myplot.add_curve("Curve1", xdata, ydata, .1, "blue")
        
    xdata = [-1.0*x+1 for x in range(100)] #That's -1 to -100 in steps of 1
    ydata = [math.cos(x) for x in xdata]    
    id = myplot.add_curve("Curve2", xdata, ydata, 2, "green")
     
    xdata = [-0.7*x for x in range(1000)] #That's 0 to 10 in steps of 0.01
    ydata = [math.tan(x) for x in xdata]    
    id = myplot.add_curve("Curve3", xdata, ydata, 1, "cyan")

    xdata = [20*x for x in range(1000)] #That's 0 to 10 in steps of 0.01
    ydata = [math.sqrt(x) for x in xdata]    
    id = myplot.add_curve("Curve4", xdata, ydata, 1, "purple")

    # Add a bunch of plots
    plotmore = True
    i=0
    while plotmore:
          xdata = [20*x for x in range(1000)] #That's 0 to 10 in steps of 0.01
          ydata = [math.sqrt(x)+i*10 for x in xdata]    
          id = myplot.add_curve("Curve4", xdata, ydata, 1, "purple")
          i=i+1
          if (i==40):
              plotmore=False
        
    myplot.delete_curve("Curve3")   

    parseCommands=True
    while parseCommands:
        # Maintain list of plots

        command = raw_input("GrizIt> ")
        myplot.setPlotTitle("Plot Title", command, command)
        myplot.update_figure()
        #end else
    #end while
