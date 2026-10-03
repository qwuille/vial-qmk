"""Register the Replicazeron navigation workbench and toggle command."""

import FreeCADGui as Gui

try:
    from . import controller
except ImportError:
    import controller

try:
    from PySide import QtGui, QtWidgets
except ImportError:
    from PySide2 import QtGui, QtWidgets

_menu_actions = []


class ReplicazeronSettingsCommand:
    def GetResources(self):
        return {
            "MenuText": "Replicazeron settings",
            "ToolTip": "Configure Replicazeron orbit, rotate, pan, and axis directions",
        }

    def Activated(self):
        import FreeCADGui as cad_gui
        import controller as cad_controller
        try:
            from PySide import QtWidgets as qt_widgets
        except ImportError:
            from PySide2 import QtWidgets as qt_widgets

        settings = cad_controller.get_settings()
        dialog = qt_widgets.QDialog(cad_gui.getMainWindow())
        dialog.setWindowTitle("Replicazeron settings")
        layout = qt_widgets.QFormLayout(dialog)

        invert_horizontal = qt_widgets.QCheckBox("Reverse left / right")
        invert_horizontal.setChecked(bool(settings["invertHorizontal"]))
        layout.addRow(invert_horizontal)
        invert_vertical = qt_widgets.QCheckBox("Reverse up / down")
        invert_vertical.setChecked(bool(settings["invertVertical"]))
        layout.addRow(invert_vertical)

        rotation_rate = qt_widgets.QDoubleSpinBox()
        rotation_rate.setRange(5.0, 360.0)
        rotation_rate.setSingleStep(5.0)
        rotation_rate.setSuffix(" deg/s")
        rotation_rate.setValue(float(settings["rotationDegreesPerSecond"]))
        layout.addRow("Orbit / rotate speed", rotation_rate)

        pan_rate = qt_widgets.QDoubleSpinBox()
        pan_rate.setRange(0.05, 5.0)
        pan_rate.setSingleStep(0.05)
        pan_rate.setValue(float(settings["panScalePerSecond"]))
        layout.addRow("Pan speed", pan_rate)

        toggle_pan = qt_widgets.QCheckBox("Toggle CAD Pan instead of hold")
        toggle_pan.setChecked(bool(settings["togglePanMode"]))
        layout.addRow(toggle_pan)

        toggle_rotate = qt_widgets.QCheckBox("Toggle CAD Rotate instead of hold")
        toggle_rotate.setChecked(bool(settings["toggleRotateMode"]))
        layout.addRow(toggle_rotate)

        start_bridge = qt_widgets.QCheckBox("Start bridge automatically")
        start_bridge.setChecked(bool(settings["startBridgeAutomatically"]))
        layout.addRow(start_bridge)
        buttons = qt_widgets.QDialogButtonBox(qt_widgets.QDialogButtonBox.Ok | qt_widgets.QDialogButtonBox.Cancel)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addRow(buttons)

        if dialog.exec() == qt_widgets.QDialog.Accepted:
            settings["invertHorizontal"] = invert_horizontal.isChecked()
            settings["invertVertical"] = invert_vertical.isChecked()
            settings["rotationDegreesPerSecond"] = rotation_rate.value()
            settings["panScalePerSecond"] = pan_rate.value()
            settings["togglePanMode"] = toggle_pan.isChecked()
            settings["toggleRotateMode"] = toggle_rotate.isChecked()
            settings["startBridgeAutomatically"] = start_bridge.isChecked()
            cad_controller.save_settings(settings)

    def IsActive(self):
        return True


class ReplicazeronToggleCommand:
    def GetResources(self):
        return {
            "MenuText": "Replicazeron navigation",
            "ToolTip": "Enable or disable pointer-independent Replicazeron viewport rotation",
        }

    def Activated(self):
        import controller as cad_controller
        cad_controller.toggle()

    def IsActive(self):
        return True

    def IsChecked(self):
        import controller as cad_controller
        return cad_controller.is_running()


class ReplicazeronWorkbench(Workbench):
    MenuText = "Replicazeron CAD"
    ToolTip = "Replicazeron analog viewport navigation"

    def Initialize(self):
        import controller as cad_controller
        commands = ["Replicazeron_ToggleNavigation", "Replicazeron_Settings"]
        self.appendToolbar("Replicazeron", commands)
        self.appendMenu("Replicazeron", commands)
        cad_controller.start()

    def Activated(self):
        import controller as cad_controller
        cad_controller.start()

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addCommand("Replicazeron_ToggleNavigation", ReplicazeronToggleCommand())
Gui.addCommand("Replicazeron_Settings", ReplicazeronSettingsCommand())
Gui.addWorkbench(ReplicazeronWorkbench())

# Keep settings discoverable even while another workbench is selected.
menu_bar = Gui.getMainWindow().menuBar()
replicazeron_menu = menu_bar.findChild(QtWidgets.QMenu, "ReplicazeronGlobalMenu")
if replicazeron_menu is None:
    replicazeron_menu = menu_bar.addMenu("Replicazeron")
    replicazeron_menu.setObjectName("ReplicazeronGlobalMenu")
    navigation_action = QtGui.QAction("Replicazeron navigation", replicazeron_menu)
    navigation_action.triggered.connect(
        lambda _checked=False, run=Gui.runCommand: run("Replicazeron_ToggleNavigation")
    )
    settings_action = QtGui.QAction("Replicazeron settings", replicazeron_menu)
    settings_action.triggered.connect(
        lambda _checked=False, run=Gui.runCommand: run("Replicazeron_Settings")
    )
    replicazeron_menu.addAction(navigation_action)
    replicazeron_menu.addAction(settings_action)
    _menu_actions.extend((navigation_action, settings_action))

# FreeCAD imports InitGui.py while loading installed modules. Start navigation
# here so users do not have to select the Replicazeron workbench each session.
controller.start()
