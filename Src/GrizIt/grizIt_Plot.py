#!/usr/bin/python
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

import os, sys, re, bisect, math, copy, time, csv

import numpy as np
from numpy import arange, sin, pi
import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.font_manager import FontProperties

font0 = FontProperties()

import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

from PySide.QtCore import *
from PySide.QtGui import *
import PySide.QtXml

import grizIt_Session
import grizIt_GuiMain as GUI

session = grizIt_Session.session
env     = grizIt_Session.env

command=""
timer = QtCore.QTimer()

################
# Misc Commands
################

# This Class fixes a problem with the Nav Toolbar not being
# able to load files ending in .svg. We just rename them
# to png and the icons will load properly

class VMToolbar(NavigationToolbar):
    def __init__(self, plotCanvas, parent):
        NavigationToolbar.__init__(self, plotCanvas, parent)

    def _icon(self, name):
        #dirty hack to use exclusively .png and thus avoid .svg usage
        #because .exe generation is problematic with .svg
        name = name.replace('.svg','.png')
        return QIcon(os.path.join(self.basedir, name))
    #end def
    
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
            item.setCheckState(QtCore.Qt.Unchecked)
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
        self.canvas.draw()
    #end def
    
    def update_plot_attributes(self):
        self.axes.set_color_cycle(['c', 'm', 'y', 'k'])
        font=self.makeFont("times", 12, "normal", "green")
        self.axes.set_title(self.plotTitle, FontProperties="font")
        xtext = self.axes.set_xlabel(self.xTitle) # returns a Text instance
        ytext = self.axes.set_ylabel(self.yTitle)
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

                xdata, ydata = self.series.get_series_data(name, x_from, x_to)

                marker=""
                if (self.marker_cb.isChecked()):
                    marker="o"
            
                self.axes.plot(xdata, ydata, 'o-', label=name, linewidth=lw, color=color, marker=marker, markerfacecolor="blue", markersize=4)

        if has_series and self.legend_cb.isChecked():
           self.axes.legend()

        if (has_series and self.grid_cb.isChecked()):
            self.axes.grid(True)
        else:
            self.axes.grid(False)

        self.canvas.draw()
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
        
        self.dpi = 175
        self.fig = Figure((8.0, 4.0), dpi=self.dpi)
        self.canvas = FigureCanvas(self.fig)
        self.canvas.setParent(self.main_frame)
        
        self.axes = self.fig.add_subplot(111)
        self.mpl_toolbar = VMToolbar(self.canvas, self.main_frame)
        
        log_label = QLabel("Data series:")
        self.series_list_view = QListView()
        self.series_list_view.setModel(self.series_list_model)
        self.series_list_view.clicked.connect(self.on_list_clicked)
        
        spin_label1 = QLabel('X from')
        self.from_spin = QDoubleSpinBox()
        self.from_spin.setSingleStep(1.0)
        self.from_spin.setDecimals(8)
        self.from_spin.setRange(0, 100)
        self.from_spin.setEnabled(True)
        
        spin_label2 = QLabel('to')
        self.to_spin = QDoubleSpinBox()
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
        
        show_button = QPushButton("&Show")
        self.connect(show_button, SIGNAL('clicked()'), self.on_show)

        spin_hboxIncrement = QHBoxLayout()
        spin_label3 = QLabel('X Range Increment')
        self.spin_incrementInput = QtGui.QLineEdit()
        spin_hboxIncrement.addWidget(spin_label3)
        spin_hboxIncrement.addWidget(self.spin_incrementInput) 

        self.connect(self.spin_incrementInput, SIGNAL('returnPressed()'),   self.update_increment)
        self.connect(self.spin_incrementInput, SIGNAL('editingFinished()'), self.update_increment)
               
        left_vbox = QVBoxLayout()
        left_vbox.addWidget(self.canvas)
        left_vbox.addWidget(self.mpl_toolbar)

        right_vbox = QVBoxLayout()
        right_vbox.addWidget(log_label)
        right_vbox.addWidget(self.series_list_view)
        right_vbox.addLayout(spin_hboxIncrement)
        right_vbox.addLayout(spin_hbox)
        right_vbox.addLayout(show_hbox)
        right_vbox.addWidget(show_button)
        right_vbox.addStretch(1)
        
        hbox = QHBoxLayout()
        hbox.addLayout(left_vbox)
        hbox.addLayout(right_vbox)
        self.main_frame.setLayout(hbox)

        self.setCentralWidget(self.main_frame)
        self.start_timer()
    #end def

    def update_increment(self):
        self.xIncrement = str(self.spin_incrementInput.text())
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

    def makeFont(self, font, size, style, color):
        font = FontProperties()
        font.set_style('italic')
        font.set_weight('bold')
        font.set_size('x-small')
        return font
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
    id = myplot.add_curve("Curve1", xdata, ydata, 4, "blue")
        
    xdata = [-1.0*x+1 for x in range(100)] #That's -1 to -100 in steps of 1
    ydata = [math.cos(x) for x in xdata]    
    id = myplot.add_curve("Curve2", xdata, ydata, 2, "green")
     
    xdata = [-0.7*x for x in range(1000)] #That's 0 to 10 in steps of 0.01
    ydata = [math.tan(x) for x in xdata]    
    id = myplot.add_curve("Curve3", xdata, ydata, 1, "cyan")

    xdata = [20*x for x in range(1000)] #That's 0 to 10 in steps of 0.01
    ydata = [math.sqrt(x) for x in xdata]    
    id = myplot.add_curve("Curve4", xdata, ydata, 1, "purple")

    myplot.delete_curve("Curve3")   

    parseCommands=True
    while parseCommands:
        # Maintain list of plots

        command = raw_input("GrizIt> ")
        myplot.setPlotTitle("Plot Title", command, command)
        myplot.update_figure()
        #end else
    #end while
 

"""
color='green', linestyle='dashed', marker='o',
     markerfacecolor='blue', markersize=12).  See
     :class:`~matplotlib.lines.Line2D` for detail          plt.legend(('Curve1'))

leg = ax.legend(('Model length', 'Data length', 'Total message length'),
           'upper center', shadow=True)
ax.set_ylim([-1,20])
ax.grid(False)
ax.set_xlabel('Model complexity --->')
ax.set_ylabel('Message length --->')
ax.set_title('Minimum Message Length')

ax.set_yticklabels([])
ax.set_xticklabels([])

# set some legend properties.  All the code below is optional.  The
# defaults are usually sensible but if you need more control, this
# shows you how

# the matplotlib.patches.Rectangle instance surrounding the legend
frame  = leg.get_frame()  
frame.set_facecolor('0.80')    # set the frame face color to light gray

# matplotlib.text.Text instances
for t in leg.get_texts():
    t.set_fontsize('small')    # the legend text fontsize

# matplotlib.lines.Line2D instances
for l in leg.get_lines():
    l.set_linewidth(1.5)  # the legend line width
"""
