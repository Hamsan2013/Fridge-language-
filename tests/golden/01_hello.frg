// Official FRIDGE v0.3.0 Verification Script
remember creator as "Hamsan A"
remember score as 100
add 25 to score

show "Welcome to FRIDGE Programming Language!"
show "Created by: " joined with creator

if score is greater than 100
    show "Score check passed: " joined with score
otherwise
    show "Keep exploring."
end if

remember fruits as list containing "Apple", "Mango", "Orange"
show "First fruit (1-based): " joined with first item in fruits

create function called double_number taking val returning number
    return val multiplied by 2
end function

remember result as call double_number with 21
show "Function result: " joined with result

add OBJ player_robot
    child {
        add OBJ laser_arm
    }
end OBJ

move world environment player_robot by vector3(0, 5, 10)
