#include "main.h"
#include <pthread.h>

/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
void on_center_button() {
	static bool pressed = false;
	pressed = !pressed;
	if (pressed) {
		pros::lcd::set_text(2, "I was pressed!");
	} else {
		pros::lcd::clear_line(2);
	}
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::lcd::initialize();
	pros::lcd::set_text(1, "Hello PROS User!");

	pros::lcd::register_btn1_cb(on_center_button);
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */

void *inc_x(void *x_void_ptr)
{
	/* increment x to 100 */
	int *x_ptr = (int *)x_void_ptr;
	while(++(*x_ptr) < 100);

	pros::lcd::print(2,"x increment finished, %d\n", *x_ptr);

  //Delay to test pthread_join functionality
  pros::delay(5000);
  pros::lcd::print(4,"delay F I N I S H E D");
  pros::delay(2000);

	/* the function must return something - NULL will do */
	return NULL;
}

void opcontrol() {
  pros::lcd::print(0, "Bruh");
	pros::delay(2000);
	pros::Controller master(pros::E_CONTROLLER_MASTER);
	pros::Motor left_mtr(1);
	pros::Motor right_mtr(2);
	int x = 0;
	int y = 0;

	while (true) {

		/* show the initial values of x and y */
		pros::lcd::print(0,"x: %d, y: %d\n", x, y);
		pros::delay(1000);

		/* this variable is our reference to the second thread */
		pthread_t inc_x_thread;
		pros::delay(1000);
		pros::lcd::print(1,"Thread variable created");
		pros::delay(1000);

		/* create a second thread which executes inc_x(&x) */
		if(pthread_create(&inc_x_thread, NULL, inc_x, (void*) &x)) {
			pros::lcd::print(1, "Error creating thread\n");
			pros::delay(1000);
		}

    pros::lcd::print(2, "Thread exec worked just fine");
    pros::delay(1000);
		/* increment y to 100 in the first thread */
		while(++y < 100);

		pros::lcd::print(1,"y increment finished\n");
		pros::delay(1000);

    //pros::delay(8000);
		/* wait for the second thread to finish */
		if(pthread_join(inc_x_thread, NULL)) {
			pros::lcd::print(3, "Error joining thread\n");
		}

		/* show the results - x is now 100 thanks to the second thread */
		pros::lcd::print(4,"x: %d, y: %d\n", x, y);

		pros::delay(20);
    break;
	}
  pros::lcd::print(5, "Program ending...");
  //opcontrol();
  //pros::Controller master(pros::E_CONTROLLER_MASTER);
  //pros::Motor left_mtr(1);
  //pros::Motor right_mtr(2);
  //pros::lcd::print(1, "Henlo world!");

  //while (true) {
    //pros::lcd::print(0, "%d %d %d", (pros::lcd::read_buttons() & LCD_BTN_LEFT) >> 2,
                     //(pros::lcd::read_buttons() & LCD_BTN_CENTER) >> 1,
                     //(pros::lcd::read_buttons() & LCD_BTN_RIGHT) >> 0);
    //int left = master.get_analog(ANALOG_LEFT_Y);
    //int right = master.get_analog(ANALOG_RIGHT_Y);

    //left_mtr = left;
    //right_mtr = right;
    //pros::delay(20);
  //}
}
