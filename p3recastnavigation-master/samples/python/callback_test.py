'''
Created on Sep 12, 2016

@author: consultit
'''

from panda3d.core import load_prc_file_data, LPoint3f, ClockObject
from p3recastnavigation import RNNavMeshManager, RNNavMesh
from direct.showbase.ShowBase import ShowBase

dataDir = "../data"

crowdAgent = None

globalClock = None

def crowdAgentCallback(crowdAgent):
    """crowd agent update callback function"""
    
    print(crowdAgent.get_name() + " params:")
    print(crowdAgent.get_params())

def navMeshCallback(navMesh):
    """nav mesh update callback function"""
    
    global globalClock
    print(navMesh.get_name() + " real time and dt: "
          + str(globalClock.get_real_time()) + str(globalClock.get_dt()))

if __name__ == '__main__':
    # Load your application's configuration
    load_prc_file_data("", "model-path " + dataDir)
    load_prc_file_data("", "win-size 1024 768")
    load_prc_file_data("", "show-frame-rate-meter #t")
    load_prc_file_data("", "sync-video #t")
        
    # Setup your application
    app = ShowBase()
       
    # # here is room for your own code
    print("create a nav mesh manager")
    navMesMgr = RNNavMeshManager()

    print("reparent the reference node to render")
    navMesMgr.get_reference_node_path().reparent_to(app.render)

    print("get a sceneNP as owner model")
    sceneNP = app.loader.load_model("nav_test.egg")
    
    print("create a nav mesh (it is attached to the reference node)")
    navMeshNP = navMesMgr.create_nav_mesh()
    navMesh = navMeshNP.node()
    navMesh.set_update_callback(navMeshCallback)
    
    print("mandatory: set sceneNP as owner of navMesh")
    navMesh.set_owner_node_path(sceneNP)
    
    print("setup the navMesh with sceneNP as its owner object")
    navMesh.setup()

    print("reparent sceneNP to the reference node")
    sceneNP.reparent_to(navMesMgr.get_reference_node_path())
    
    print("get the agent model")
    agentNP = app.loader.load_model("eve.egg")
    agentNP.set_scale(0.40)

    print("create the crowd agent (it is attached to the reference node) and set its position")
    crowdAgentNP = navMesMgr.create_crowd_agent("crowdAgent")
    crowdAgent = crowdAgentNP.node()
    crowdAgentNP.set_pos(24.0, -20.4, -2.37)
    crowdAgent.set_update_callback(crowdAgentCallback)
    globalClock = ClockObject.get_global_clock()
    
    print("attach the agent model to crowdAgent")
    agentNP.reparent_to(crowdAgentNP)
    
    print("attach the crowd agent to the nav mesh")
    navMesh.add_crowd_agent(crowdAgentNP)
    print(str(crowdAgent) + " added to: " + str(crowdAgent.get_nav_mesh()))

    print("start the path finding default update task")
    navMesMgr.start_default_update()

    print("DEBUG DRAWING: make the debug reference node path sibling of the reference node")
    navMesMgr.get_reference_node_path_debug().reparent_to(app.render)
    print("enable debug drawing")
    navMesh.enable_debug_drawing(app.camera)

    print("toggle debug draw")
    navMesh.toggle_debug_drawing(True)
    
    print("set crowd agent move target on scene surface")
    crowdAgent.set_move_target(LPoint3f(-20.5, 5.2, -2.36))
    
    # place camera
    trackball = app.trackball.node()
    trackball.set_pos(-10.0, 90.0, -2.0)
    trackball.set_hpr(0.0, 15.0, 0.0)
   
    # app.run(), equals to do the main loop in C++
    app.run()

