// Official FRIDGE Test Program
remember creator as "Hamsan A"
show "Welcome to FRIDGE Programming Language!"
show "Created by: " joined with creator

repeat 2 times
    show "Simple English. Real programs."
end repeat

add OBJ player_robot
    child {
        add OBJ laser_arm
    }
end OBJ

move world environment player_robot by vector3(0, 5, 10)
listen for key "W"
play sound "assets/sounds/startup.wav"
