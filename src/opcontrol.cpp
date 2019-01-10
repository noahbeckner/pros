#include "main.h"
#include <pthread.h>

void *inc_x(void *x_void_ptr)
{
	/* increment x to 100 */
	int *x_ptr = (int *)x_void_ptr;
	while(++(*x_ptr) < 100);

	pros::lcd::print(2,"x increment finished\n");

	/* the function must return something - NULL will do */
	return NULL;
}
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
void opcontrol() {
	pros::lcd::print(0,"opcontrol started");
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
		if(pthread_create(&inc_x_thread, NULL, inc_x, &x)) {

			pros::lcd::print(1, "Error creating thread\n");
			pros::delay(1000);

		}
		/* increment y to 100 in the first thread */
		while(++y < 100);

		pros::lcd::print(1,"y increment finished\n");
		pros::delay(1000);

		/* wait for the second thread to finish */
		if(pthread_join(inc_x_thread, NULL)) {

			pros::lcd::print(3, "Error joining thread\n");

		}

		/* show the results - x is now 100 thanks to the second thread */
		pros::lcd::print(4,"x: %d, y: %d\n", x, y);

		pros::delay(20);
	}
}
