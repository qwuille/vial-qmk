"""Register the Replicazeron navigation workbench and toggle command."""

import FreeCADGui as Gui

try:
    from . import controller
except ImportError:
    import controller


class ReplicazeronToggleCommand:
    def GetResources(self):
        return {
            "MenuText": "Replicazeron navigation",
            "ToolTip": "Enable or disable pointer-independent Replicazeron viewport rotation",
        }

    def Activated(self):
        controller.toggle()

    def IsActive(self):
        return True

    def IsChecked(self):
        return controller.is_running()


class ReplicazeronWorkbench(Workbench):
    MenuText = "Replicazeron CAD"
    ToolTip = "Replicazeron analog viewport navigation"

    def Initialize(self):
        Gui.addCommand("Replicazeron_ToggleNavigation", ReplicazeronToggleCommand())
        self.appendToolbar("Replicazeron", ["Replicazeron_ToggleNavigation"])
        self.appendMenu("Replicazeron", ["Replicazeron_ToggleNavigation"])
        controller.start()

    def Activated(self):
        controller.start()

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(ReplicazeronWorkbench())
