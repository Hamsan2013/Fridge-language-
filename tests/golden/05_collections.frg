// ============================================
// Official FRIDGE v0.4.0 Verification Program
// Creator: Hamsan A
// ============================================

show "Starting FRIDGE v0.4.0 Engine..."

remember creator as "Hamsan A"
show "Toolchain author: " joined with creator

// 1. Algebraic Operator Precedence
remember base_val as 10
remember math_res as base_val plus 5 multiplied by 4
show "Precedence Check (Expected 30): " joined with math_res

// 2. Parentheses Evaluation
remember paren_res as (base_val plus 5) multiplied by 4
show "Parentheses Check (Expected 60): " joined with paren_res

// 3. Conditionals & Logical Comparisons
if math_res is at least 30 and paren_res is greater than 50
    show "Boolean Logic: VERIFIED"
otherwise
    show "Boolean Logic: FAILED"
end if

// 4. Collections & 1-Based Indices
remember weapons as list containing "Sword", "Shield", "Bow"
show "First Item: " joined with first item in weapons
show "Item 2: " joined with item 2 of weapons

// 5. Function Declaration & Nested Recursion
create function called multiply_two taking a and b returning number
    return a multiplied by b
end function

remember calc_result as call multiply_two with 7, 6
show "Function Invocation (7 * 6 = 42): " joined with calc_result

// 6. Staged 3D Engine Initialization
add OBJ player_avatar
child {
    add OBJ weapon_model
}

show "FRIDGE Toolchain operational across all platforms."
