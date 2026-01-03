#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
#include "address_map_niosv.h"


#define TIME 2500000 //time constant for 1 second delay


#define display 0x00000000 //constant defined for the HEX's to display
typedef uint16_t pixel_t;

//typedef a 32 bit consant
typedef uint32_t device;

volatile pixel_t* pVGA = (pixel_t*)FPGA_PIXEL_BUF_BASE;
volatile uint32_t* mtime_ptr = (uint32_t*)MTIMER_BASE;
volatile device* key = (device*)KEY_BASE;
volatile device* hex = (device*)HEX3_HEX0_BASE;
volatile device* sw = (device*)SW_BASE;
volatile device* led = (device*)LEDR_BASE;
device ik = 0;
device* prevkey = &ik;
volatile bool game_running = true;

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;






/* STARTER CODE BELOW. FEEL FREE TO DELETE IT AND START OVER */

void drawPixel(int y, int x, pixel_t colour)
{
	*(pVGA + (y << YSHIFT) + x) = colour;
}

pixel_t makePixel(uint8_t r8, uint8_t g8, uint8_t b8)
{
	// inputs: 8b of each: red, green, blue
	const uint16_t r5 = (r8 & 0xf8) >> 3; // keep 5b red
	const uint16_t g6 = (g8 & 0xfc) >> 2; // keep 6b green
	const uint16_t b5 = (b8 & 0xf8) >> 3; // keep 5b blue
	return (pixel_t)((r5 << 11) | (g6 << 5) | b5);
}

void rect(int y1, int y2, int x1, int x2, pixel_t c)
{
	for (int y = y1; y < y2; y++)
		for (int x = x1; x < x2; x++)
			drawPixel(y, x, c);
}


//struct for player
typedef struct {
	int x, y; //current postition
	int dx, dy; //change
	int direction; //defined 1-4, controls 90 degree turns for dx dy.
	pixel_t colour; // colour of the player
	bool alive; // if alive 1, else 0
	bool isAI; //if player =0, ai = 1
	int score;
}Player;

//define two players as global variables, avoid too many problems
Player p1;
Player p2;


//function for reading the pixel, determining collision with border
//light rail, or nothing.
pixel_t readPixel(int y, int x) {
	return *(pVGA + (y << YSHIFT) + x);
}

//function for initializing the struct
void initplayers(void) {
	//start by initializing the players, gonna comment out
	//the ai first
	p1.x = MAX_X / 3;
	p1.y = MAX_Y / 2;
	p1.dx = 1;
	p1.dy = 0;
	p1.direction = 1;
	p1.colour = blu;
	p1.alive = true;
	p1.isAI = false;

	//ai
	p2.x = 2 * MAX_X / 3;
	p2.y = MAX_Y / 2;
	p2.dx = -1;
	p2.dy = 0;
	p2.direction = 3;
	p2.colour = red;
	p2.alive = true;
	p2.isAI = true;

}

device decoder(int score1, int score2) {
	device temp = display; //local stack variable to hold updated score every round, terminate after function call
	device s1, s2;
	//score 1 decoder for HEX2
	if (score1 == 0) {
		s1 = 0x003F0000;
	}
	else if (score1 == 1) {
		s1 = 0x00060000;
	}
	else if (score1 == 2) {
		s1 = 0x005B0000;
	}
	else if (score1 == 3) {
		s1 = 0x004F0000;
	}
	else if (score1 == 4) {
		s1 = 0x00660000;
	}
	else if (score1 == 5) {
		s1 = 0x006D0000;
	}
	else if (score1 == 6) {
		s1 = 0x007D0000;
	}
	else if (score1 == 7) {
		s1 = 0x00070000;
	}
	else if (score1 == 8) {
		s1 = 0x007F0000;
	}
	else {
		s1 = 0x00670000;
	}

	//score 2 decoder for HEX0
	if (score2 == 0) {
		s2 = 0x0000003F;
	}
	else if (score2 == 1) {
		s2 = 0x00000006;
	}
	else if (score2 == 2) {
		s2 = 0x0000005B;
	}
	else if (score2 == 3) {
		s2 = 0x0000004F;
	}
	else if (score2 == 4) {
		s2 = 0x00000066;
	}
	else if (score2 == 5) {
		s2 = 0x0000006D;
	}
	else if (score2 == 6) {
		s2 = 0x0000007D;
	}
	else if (score2 == 7) {
		s2 = 0x00000007;
	}
	else if (score2 == 8) {
		s2 = 0x0000007F;
	}
	else {
		s2 = 0x00000067;
	}

	temp = s1 + s2;

	return temp;
}

double gamespeed(volatile device* sw) {
	double speed = 1;
	if ((*sw) << 22 == 0x00400000) {
		speed = 220 * TIME;
	}
	else if ((*sw) << 22 == 0x00800000) {
		speed = 3.5 * TIME;
	}
	else if ((*sw) << 22 == 0x01000000) {
		speed = 3 * TIME;
	}
	else if ((*sw) << 22 == 0x02000000) {
		speed = 2 * TIME;
	}
	else if ((*sw) << 22 == 0x04000000) {
		speed = 1.5 * TIME;
	}
	else if ((*sw) << 22 == 0x08000000) {
		speed = 0.95 * TIME;
	}
	else if ((*sw) << 22 == 0x10000000) {
		speed = 0.925 * TIME;
	}
	else if ((*sw) << 22 == 0x20000000) {
		speed = 0.9 * TIME;
	}
	else if ((*sw) << 22 == 0x40000000) {
		speed = 0.85 * TIME;
	}
	else if ((*sw) << 22 == 0x80000000) {
		speed = 0.8 * TIME;
	}
	else {
		speed = TIME;
	}
	return speed;
}


//timer interuppt functions

void set_mtimer(volatile uint32_t* time_ptr, uint64_t new_time64)
{
	*(time_ptr) = (uint32_t)0;
	// prevent hi from increasing before setting lo
	*(time_ptr + 1) = (uint32_t)(new_time64 >> 32);
	// set hi part
	*(time_ptr) = (uint32_t)new_time64;
	// set lo part
}

uint64_t get_mtimer(volatile uint32_t* time_ptr) {
	uint32_t mtime_h, mtime_l;
	//can only read 32b at a time
	//hi part may increment between reading hin and lo
	//if it increments, re-read lo and hi again

	do {
		mtime_h = *(time_ptr + 1);
		mtime_l = *(time_ptr);
	} while (mtime_h != *(time_ptr + 1));
	// if mtime_hi has changed, repeat

	//return 64b result
	return ((uint64_t)mtime_h << 32) | mtime_l;
}

void setup_mtimecmp() {

	uint64_t mtime64 = get_mtimer(mtime_ptr);
	// read current mtime (counter)

	mtime64 = (mtime64 / (uint32_t)gamespeed(sw) + 1) * (uint32_t)gamespeed(sw);
	// compute end of next time PERIOD

	set_mtimer(mtime_ptr + 2, mtime64);
	// write first mtimecmp ("+2" == mtimecmp)
}

#define KEY_IRQ_MASK   (*(volatile uint32_t*)(KEY_BASE + 0x8)) //type cast so key irq mask and edge can be deferenced
#define KEY_EDGE_CAP   (*(volatile uint32_t*)(KEY_BASE + 0xC))

void init_key_irq(void) {
	KEY_EDGE_CAP = 0xF;     // clear any stale edges
	KEY_IRQ_MASK = 0x3;     // enable IRQs on KEY0 and KEY1, didn't use bit masking since only those two keys are relevant anyways.
}

bool doublep = false; //global variable defined, acts as a small state machine to detect double presses.

void mkey_ISR(void) {
	uint32_t edges = KEY_EDGE_CAP;  // read which key(s) caused the IRQ
	KEY_EDGE_CAP = edges; // rewrite to edgecap reg to clear the edges so it can be used for next IRQ


	if ((edges == 0x1) && (doublep == false)) {
		//turn on led[0] to signal a right turn
		*led = 0x1;
		doublep = true; // set true, mtime will reset if timer goes before second press
	}
	else if ((edges == 0x2) && (doublep == false)) {
		*led = 0x2;
		doublep = true;
	}
	else if ((edges == 0x1) && (doublep == true)) {
		//cancel turns because if double press, doublep will still be true
		*led = 0;

	}
	else if ((edges == 0x2) && (doublep == true)) {
		*led = 0;

	}
	else {
		;//do nothing if both are pressed or both unpressed
	}


}

void mtimer_ISR(void) {

	uint64_t mtimecmp64 = get_mtimer(mtime_ptr + 2);
	// read mtimecmp

	mtimecmp64 += (uint32_t)gamespeed(sw);
	// time of future irq = one period in future

	set_mtimer(mtime_ptr + 2, mtimecmp64);
	// write next mtimecmp

	if (!game_running) return; //if main hasn't set this back to true, then timer interrupts won't fire because game //hasn't been reinitialized.

	//side effect goes here

	pixel_t colour, colourai;
	int dir = 3;
	int xai, yai;

	if ((*led) == 0x1) { //turns left (CCW)
		if (p1.direction == 1) {
			p1.direction = 4;
		}
		else {
			p1.direction--; 
		}

	}
	else if ((*led) == 0x2) { //turns right (CCW)
		if (p1.direction == 4) {
			p1.direction = 1;
		}
		else {
			p1.direction++;
		}

	}
	else {
		; //LED are off, keepo going same direction
	}

	//change dx,dy according to direction or it stays the same
	if (p1.direction == 1) {
		p1.dx = 1;
		p1.dy = 0;
	}
	else if (p1.direction == 2) {
		p1.dx = 0;
		p1.dy = -1;
	}
	else if (p1.direction == 3) {
		p1.dx = -1;
		p1.dy = 0;
	}
	else {
		p1.dx = 0;
		p1.dy = 1;
	}



	//move manual player
	p1.x = p1.x + p1.dx;
	p1.y = p1.y + p1.dy;
	colour = readPixel(p1.y, p1.x);
	if (colour == blk) {
		drawPixel(p1.y, p1.x, p1.colour);
		*led = 0; //clear turn directions
		doublep = false; //reset so next press isn't counted as double press.
	}
	else {
		p1.alive = false;
		p2.score++;
		game_running = false;
		return;
	}


	//start of ai 
	//ai always looks one pixels ahead, but can readily adjust for any amount
	colourai = readPixel(p2.y + p2.dy, p2.x + p2.dx);
	dir = p2.direction;
	xai = p2.dx;
	yai = p2.dy;

	if (colourai != blk) { //first look ahead and see if you need to turn 

		if (p2.direction == 4) {
			dir = 1;
		}
		else {
			dir++;
		}

		if (dir == 1) {
			xai = 1;
			yai = 0;
		}
		else if (dir == 2) {
			xai = 0;
			yai = -1;
		}
		else if (dir == 3) {
			xai = -1;
			yai = 0;
		}
		else {
			xai = 0;
			yai = 1;
		}
		colourai = readPixel(p2.y + yai, p2.x + xai);

		if (colourai == blk) { //try to turn left first
			p2.dy = yai;
			p2.dx = xai;
			p2.direction = dir;
			p2.x = p2.x + p2.dx;
			p2.y = p2.y + p2.dy;
			drawPixel(p2.y, p2.x, p2.colour);
		}
		else { //left didnt work, so adjust for a right turn attempt
			dir = p2.direction;
			xai = p2.dx;
			yai = p2.dy;
			if (p2.direction == 1) {
				dir = 4;
			}
			else {
				dir--;
			}
			if (dir == 1) {
				xai = 1;
				yai = 0;
			}
			else if (dir == 2) {
				xai = 0;
				yai = -1;
			}
			else if (dir == 3) {
				xai = -1;
				yai = 0;
			}
			else {
				xai = 0;
				yai = 1;
			}
			colourai = readPixel(p2.y + yai, p2.x + xai);

			if (colourai == blk) { //try to turn right
				p2.dy = yai;
				p2.dx = xai;
				p2.direction = dir;
				p2.x = p2.x + p2.dx;
				p2.y = p2.y + p2.dy;
				drawPixel(p2.y, p2.x, p2.colour);
			}
			else { // go straight
				p2.x = p2.x + p2.dx;
				p2.y = p2.y + p2.dy;
				drawPixel(p2.y, p2.x, p2.colour);
				p2.alive = false;
				p1.score++;
				game_running = false;
				return;
			}

		}
	}
	else { //go straight, nothing detected ahead
		p2.x = p2.x + p2.dx;
		p2.y = p2.y + p2.dy;
		drawPixel(p2.y, p2.x, p2.colour);
	}
	//finished ai		
}

// this attribute tells compiler to use "mret" rather than "ret"
// at the end of handler() function; also declares its type
static void handler(void) __attribute__((interrupt("machine")));

void handler(void) {

	int mcause_value;

	// inline assembly, links register %0 to mcause_value
	__asm__ volatile("csrr %0, mcause" : "=r"(mcause_value));

	if (mcause_value == 0x80000007) { // machine timer
		mtimer_ISR();
	}
	else if (mcause_value == 0x80000012) {     // KEY IRQ (18)
		mkey_ISR();
	}
	else {
		;// unknown cause, maybe ignore or signal error
	}
}

void setup_cpu_irqs(uint32_t new_mie_value) {

	uint32_t mstatus_value, mtvec_value, old_mie_value;

	mstatus_value = 0b1000; // interrupt bit mask

	mtvec_value = (uint32_t)&handler; // set trap address

	__asm__ volatile("csrc mstatus, %0" :: "r"(mstatus_value));
	// master irq disable

	__asm__ volatile("csrw mtvec, %0" :: "r"(mtvec_value));
	// sets handler

	__asm__ volatile("csrr %0, mie" : "=r"(old_mie_value));
	__asm__ volatile("csrc mie, %0" :: "r"(old_mie_value));
	__asm__ volatile("csrs mie, %0" :: "r"(new_mie_value));
	// reads old irq mask, removes old irqs, sets new irq mask

	__asm__ volatile("csrs mstatus, %0" :: "r"(mstatus_value));
	// master irq enable
}



void delay(uint64_t N)
{
	uint64_t timegoal = get_mtimer(mtime_ptr) + N; //articially set the cmp time

	while (timegoal > get_mtimer(mtime_ptr)) {
		*pVGA; // read volatile memory location to waste time
	}
}
int main()
{
	//some needed variables to start
	const int half_y = MAX_Y / 2;
	

	//initialize first game and interrupts
	rect(0, MAX_Y, 0, MAX_X, blk);
	rect(0, MAX_Y, 0, 1, wht);
	rect(0, MAX_Y, MAX_X - 1, MAX_X, wht);
	rect(0, 1, 0, MAX_X, wht);
	rect(MAX_Y - 1, MAX_Y, 0, MAX_X, wht);

	//this is obstacles for the vga nios
	
	rect(44,46,40,42,wht);
	rect (30,32, 21,23,wht);
	rect(half_y+2,half_y+4,70,72,wht);
	rect(half_y-10,half_y-8,130,135,wht);
	


	initplayers();
	printf("start\n");

	init_key_irq(); // need to initialize the keys for interrupts
	setup_mtimecmp();
	setup_cpu_irqs(0x40080); // enable mtimer IRQs and key

	while (p1.score < 9 && p2.score < 9) {

		*hex = decoder(p1.score, p2.score);


		//reset players and screen, also update score if somebody died

		if (!game_running) {



			rect(0, MAX_Y, 0, MAX_X, blk);
			rect(0, MAX_Y, 0, 1, wht);
			rect(0, MAX_Y, MAX_X - 1, MAX_X, wht);
			rect(0, 1, 0, MAX_X, wht);
			rect(MAX_Y - 1, MAX_Y, 0, MAX_X, wht);
			initplayers();

			//this is obstacles for the vga nios
			
			rect(44,46,40,42,wht);
			rect (30,32, 21,23,wht);
			rect(half_y+2,half_y+4,70,72,wht);
			rect(half_y-10,half_y-8,130,135,wht);
			

			game_running = true;
		}



	}
	//disable interrupts after game reaches 9, will leave infinite loop of game winner on screen
	__asm__ volatile("csrc mstatus, %0" :: "r"(0b1000));

	*hex = decoder(p1.score, p2.score); //updated for 9

	pixel_t winner;

	if (p1.score == 9) {
		winner = blu;
	}
	else {
		winner = red;
	}

	while (1) {
		rect(0, MAX_Y, 0, MAX_X, winner);
	}





	printf("done\n");
	return 0;
}