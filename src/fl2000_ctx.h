// fl2000_ctx.h
//
// (c)Copyright 20017, Fresco Logic, Incorporated.
//
// Purpose:
//

#ifndef _FL2000_CTX_H_
#define _FL2000_CTX_H_

/*
 * forward declaration
 */
struct dev_ctx;
struct render_ctx;

/*
 * schedule at least 2 video frames to the bus, to buy driver response time
 */
#define	NUM_RENDER_ON_BUS	2

struct fl2000_timing_entry {
	uint32_t 	width;
	uint32_t 	height;
	uint32_t 	freq;
	uint32_t 	h_total_time;
	uint32_t 	v_total_time;
	uint32_t 	h_sync_reg_1;
	uint32_t 	h_sync_reg_2;
	uint32_t 	v_sync_reg_1;
	uint32_t 	v_sync_reg_2;
	uint32_t 	bulk_asic_pll;
};

struct registry
{
	uint32_t CurrentNumberOfDevices;

	uint32_t CompressionEnable;
	uint32_t Usb2PixelFormatTransformCompressionEnable;
};

struct vr_params
{
	uint32_t	width;
	uint32_t	height;
	uint32_t	freq;
	uint32_t	h_total_time;
	uint32_t	v_total_time;

	// Compression.
	//
	uint32_t	use_compression;
	uint32_t	compression_mask;
	uint32_t	compression_mask_index;
	uint32_t	compression_mask_index_min;
	uint32_t	compression_mask_index_max;

	uint32_t	pll_reg;
	uint32_t	input_bytes_per_pixel;
	uint32_t	output_image_type;
	uint32_t	color_mode_16bit;
};

#define	MAX_NUM_FRAGMENT	((MAX_BUFFER_SIZE + PAGE_SIZE - 1) >> PAGE_SHIFT)

struct primary_surface {
	struct list_head 	list_entry;
	uint64_t		handle;
	uint64_t		user_buffer;
	uint32_t		buffer_length;
	uint32_t		width;
	uint32_t		height;
	uint32_t		pitch;
	uint32_t		color_format;
	uint32_t		type;
	uint32_t		frame_num;
	uint32_t		start_offset;
	uint64_t		physical_address;
	struct page *		first_page;
	uint8_t *		shadow_buffer;
	uint8_t *		mapped_buffer;	/* return from vm_map_ram */
	uint8_t *		system_buffer;	/* offset corrected buffer */
	uint8_t *		render_buffer;
	bool			pre_locked;

	struct page **		pages;
	unsigned int		nr_pages;
	int			pages_pinned;
	struct scatterlist 	sglist[MAX_NUM_FRAGMENT];
};

struct render_ctx {
	struct list_head 	list_entry;

	struct dev_ctx *	dev_ctx;
	struct primary_surface*	primary_surface;
	void *			transfer_buffer;
	uint32_t		transfer_buffer_length;
	struct urb*		main_urb;
	struct urb*		zero_length_urb;
	uint32_t		pending_count;
	struct tasklet_struct	tasklet;
};

struct render {
	struct list_head 	free_list;
	uint32_t		free_list_count;
	spinlock_t 		free_list_lock;

	struct list_head 	ready_list;
	uint32_t		ready_list_count;
	spinlock_t 		ready_list_lock;

	struct list_head 	busy_list;
	uint32_t		busy_list_count;
	spinlock_t 		busy_list_lock;

	struct render_ctx	render_ctx[NUM_OF_RENDER_CTX];

	struct list_head 	surface_list;
	uint32_t		surface_list_count;
	spinlock_t 		surface_list_lock;

	struct display_mode	display_mode;
	uint32_t		last_frame_num;
	struct primary_surface* last_updated_surface;

	uint32_t		green_light;
};

struct urb_node {
	struct list_head entry;
	struct dev_ctx *fl2k;
	struct delayed_work release_urb_work;
	struct urb *urb;
};

struct urb_list {
	struct list_head list;
	spinlock_t lock;
	struct semaphore limit_sem;
	int available;
	int count;
	size_t size;
};

struct dev_ctx {
	struct usb_device*		usb_dev;
	struct usb_device_descriptor	usb_dev_desc;
	struct kref			kref;

	/*
	 * control transfer scratch area.
	 * starting from some kernel version, the usb_control_msg no longer
	 * accept buffer from stack (which is in vmalloc area).
	 * so we have to allocate our scratch area so that the buffer
	 * usb core is happy with the buffer.
	 */
	uint32_t			ctrl_xfer_buf;

	/*
	 * some compiler (eg. arm-hisiv200-linux-gcc-4.4.1) does not provide
	 * __sync_xxx_and_fetch. kind of sucks. we use our sync lock here.
	 */
	spinlock_t			count_lock;
	struct urb_list urbs;
	atomic_t lost_pixels; /* 1 = a render op failed. Need screen refresh */

	/*
	 * bulk out interface
	 */
	struct usb_interface*		usb_ifc_streaming;
	int				usb_pipe_bulk_out;

	/*
	 * interrupt in pipe related
	 */
	struct usb_interface*		usb_ifc_intr;
	struct usb_host_endpoint*	ep_intr_in;
	struct usb_endpoint_descriptor*	ep_desc_intr_in;
	int 				ep_num_intr_in;
	int				usb_pipe_intr_in;
	struct urb*			intr_urb;
	uint8_t				intr_data;
	uint32_t			intr_pipe_pending_count;
	bool				intr_pipe_started;
	struct workqueue_struct *	intr_pipe_wq;
	struct work_struct 		intr_pipe_work;

	struct registry			registry;

	/*
	 * flags and counters
	 */
	bool				monitor_plugged_in;
	bool				dev_gone;
	uint32_t			card_name;

	bool				hdmi_chip_found;
	bool				hdmi_running_in_dvi_mode;
	bool				hdmi_powered_up;
	uint32_t			hdmi_audio_use_spdif;

	struct vr_params		vr_params;
	struct render			render;
	uint8_t				monitor_edid[8][EDID_SIZE];

	/*
	 * user mode app management
	 */
	uint32_t			open_count;
	wait_queue_head_t		ioctl_wait_q;

	/*
	 * SURFACE_TYPE_VIRTUAL_CONTIGUOUS/SURFACE_TYPE_PHYSICAL_CONTIGUOUS
	 * allocation management
	 */
	struct page * 			start_page;
};

#endif // _FL2000_CTX_H_

// eof: fl2000_ctx.h
//
