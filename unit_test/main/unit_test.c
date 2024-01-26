#include <stdio.h>
// test_example.c
#include "unity.h"
//#include "example.h"  // Include the header file containing your functions to be tested

// The function to test
int add(int a, int b) {
    return a + b;
}

// Unity test function
void test_addition(void) {
    TEST_ASSERT_EQUAL_INT(5, add(2, 3));  // Assert that add(2, 3) equals 5
    TEST_ASSERT_EQUAL_INT(10, add(7, 3)); // Assert that add(7, 3) equals 10
}

// Main function for the test
void app_main() {
    UNITY_BEGIN();

    // Run the test
    RUN_TEST(test_addition);

    UNITY_END();
}
