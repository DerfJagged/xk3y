/*
 * remoted.c - glue between hid remote and virtual framebuffer
 */

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <hid.h>

#define FBIO_WAIT_FOR_UPDATE	_IOWR('F', 0x19, unsigned long)
#define FBSIZE			1024

#define EP_OUT			0x01

#define NUM_REGIONS		32

#define POLL_RATE		40 // ms

static char const SOCKET_NAME[]	= "/var/run/remote";

static int copy_region(unsigned char *out, unsigned char const in[], unsigned int region)
{
//	static unsigned int fb_state[NUM_REGIONS][8];
	int l;//, result = 0;
	
	for (l = 0; l < 8; l++)
		((unsigned int*)out)[l] = 0xffffffff;
		
	for (l = 0; l < 8; l++)
	{
		unsigned int slice = ((unsigned int*)in)[ 32*(region >> 2) + 4*l + (region & 3) ];
		unsigned char mask;
		int bi;

		if (!slice)
			continue;
			
		slice = ntohl(slice);
		mask = ~(1 << l);
		
		for (bi = 0; slice && bi < 32; bi++, slice <<= 1)
			if (slice & 0x80000000)
				out[bi] &= mask;
	}
	
//	for (l = 0; l < 8; l++)
//	{
//		if (fb_state[region][l] == ((unsigned int*)out)[l])
//			continue;
//			
//		result = 1;
//		fb_state[region][l] = ((unsigned int*)out)[l];
//	}
	
	return 0;//result;
}

// the endpoint in the hid tree where we send our commands
static int const PATH[] = { 0xff000001, 0xff000001 };

int main(int argc, char *argv[])
{
	HIDInterfaceMatcher matcher = { 0x1843, 0xbeef, NULL, NULL, 0 };
	HIDInterface hid;
	unsigned char const *fb;
	int lpi = 0, ret, fd, d, sockfd;
	struct sockaddr_un serv_addr;
	int fail = 0;
//	int force_fb_update;

	// polling rate for button events
	unsigned long l = POLL_RATE;

	// the default button state (no buttons pressed)
	int button_state = 7;
	
	hid_set_debug(HID_DEBUG_ERRORS);
	hid_set_debug_stream(stderr);
	hid_set_usb_debug(0);

	if (!(fd = open("/dev/fb0", O_RDWR))) {
		fprintf(stderr, "open() failed");
		return 1;
	}

	fb = mmap(0, FBSIZE, PROT_READ /*| PROT_WRITE*/, MAP_SHARED, fd, 0);

	if (!fb) {
		fprintf(stderr, "mmap() failed");
		return 1;
	}

	/* ignore 'Broken pipe' signals */
	signal (SIGPIPE, SIG_IGN);
	
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{
		fprintf(stderr, "socket() failed");
		return 1;
	}
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, SOCKET_NAME);
	unlink(SOCKET_NAME);
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	{
		fprintf(stderr, "bind() failed");
		return 1;
	}

	listen(sockfd, 1);

hid_reinit:
//	force_fb_update = 1;
	
	if (hid_is_initialised())
		hid_cleanup();

	ret = hid_init();

	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_init failed with return code %d\n", ret);

		sleep(1);
		goto hid_reinit;
	}

	hid_reset_HIDInterface(&hid);
	
	ret = hid_force_open(&hid, 0, &matcher, 3);

	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_force_open failed with return code %d\n", ret);

		sleep(1);
		goto hid_reinit;
	}
	
	while (1)
	{
		static struct {
			unsigned char _unused1;
			unsigned char cmd;
			unsigned char subcmd;
		} out = { 1, 0x1f, 5 };

		static int timer;
		
		struct timeval tv = { 0, 0 };
		fd_set readfds;
		
		char input[3];
		int i, connfd = fileno(stdout);
		
		hid_set_output_report(&hid, PATH, sizeof(PATH)/sizeof(int),
			(char*)&out, sizeof(out));

		for (i = 0; i < NUM_REGIONS; i += 2)
		{
			unsigned char pixels[64];
			
			copy_region(pixels, fb, i);
			copy_region(pixels + 32, fb, i+1);
			
			usb_bulk_write(hid.dev_handle, EP_OUT, (char*)pixels, sizeof(pixels), 500);
		}

//		force_fb_update = 0;
again:
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
	
		// non-blocking check if a connection request is pending
		if (select(sockfd+1, &readfds, NULL, NULL, &tv))
		{
			printf("remote: Accepting new connection\n");
			
			// drop the old connection
			if (connfd)
				close(connfd);
				
			connfd = accept(sockfd, NULL, NULL);
				
			if (connfd < 0)
				connfd = 0;
		}
		
		if (0 != ioctl(fd, FBIO_WAIT_FOR_UPDATE, &l))
		{
			fprintf(stderr, "ioctl() failed");
			return 1;
		}
		
		// polling time has not expired, so the display must
		// have been updated
		if (l)
			continue;
			
		ret = hid_get_input_report(&hid, PATH, sizeof(PATH)/sizeof(int), input, sizeof(input));

		if (ret != HID_RET_SUCCESS) {
			fprintf(stderr, "hid_get_input_report failed with return code %d\n", ret);

			sleep(1);
			goto hid_reinit;
		}
		
		// change since last time?
		if ((d = input[1] ^ button_state))
		{
			if (connfd)
			{
				
				if ( (d & 1) && !(input[1] & 1))
					fail = (1 != write(connfd, "1", 1));
					
				if ( (d & 2) && !(input[1] & 2))
					timer = 0;
				else if ( !fail && (d & 2) && (input[1] & 2))
				{
					fail = (1 != write(connfd, timer < 10 ? "2" : "4", 1));
					lpi = 0;
				}
				if ( !fail && (d & 4) && !(input[1] & 4))
					fail = (1 != write(connfd, "3", 1));
					
				if (fail)
				{
					fail = 0;
					close(connfd);
					connfd = 0;
				}
			}
				
			button_state = input[1];
		}
		else
		{
			/* 	Check if center button has been held down long enough to generate a long push
				when it is released, send a msg to the GUI... */
			if(!(button_state & 2) && (timer >= 10) && !lpi)
			{
				lpi = 1;
				fail = (1 != write(connfd, "5", 1));
					
				if (fail)
				{
					fail = 0;
					close(connfd);
					connfd = 0;
				}
			}
		}

		l = POLL_RATE;
		timer++;
		
		goto again;
	}

	return 0;
}

