#define _GNU_SOURCE

// Credit to community members for most of the work, especially
// DualCoder for the original work
// DualCoder, snowman, Felix, Elec for profile modification


#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include <syslog.h>
#include <string.h>

#define REQ_QUERY_GPU 0xC020462A

#define OP_READ_DEV_TYPE 0x800289 // *result type is uint64_t.
#define OP_READ_PCI_ID 0x20801801 // *result type is uint16_t[4], the second
#define OP_READ_VGPUCFG 0xa0820102
// element (index 1) is the device ID, the
// forth element (index 3) is the subsystem
// ID.

#define DEV_TYPE_VGPU_CAPABLE 3

#define STATUS_OK 0
#define STATUS_TRY_AGAIN 3

typedef int( * ioctl_t)(int fd, int request, void * data);

typedef struct iodata_t {
  uint32_t unknown_1; // Initialized prior to call.
  uint32_t unknown_2; // Initialized prior to call.
  uint32_t op_type; // Operation type, see comment below.
  uint32_t padding_1; // Always set to 0 prior to call.
  void * result; // Pointer initialized prior to call.
  // Pointee initialized to 0 prior to call.
  // Pointee is written by ioctl call.
  uint32_t unknown_4; // Set to 0x10 for READ_PCI_ID and set to 4 for
  // READ_DEV_TYPE prior to call.
  uint32_t status; // Written by ioctl call. See comment below.
}
iodata_t;

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

typedef struct {
    uint32_t gpu_type;
    char card_name[32];
    char vgpu_type[160];
    char features[128];
    uint32_t max_instances;
    uint32_t num_displays;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t max_pixels;
    uint32_t frl_config;
    uint32_t cuda_enabled;
    uint32_t ecc_supported;
    uint32_t mig_instance_size;
    uint32_t multi_vgpu_supported;
    uint32_t pad1;
    uint64_t pci_id;
    uint64_t pci_device_id;
    uint64_t framebuffer;
    uint64_t mappable_video_size;
    uint64_t framebuffer_reservation;
    uint64_t encoder_capacity;
    uint64_t bar1_length;
    uint32_t frl_enabled;
    uint8_t blob[256];
    char maybe_used_feature[1156];
} __attribute__((packed)) vgpucfg_t;


int ioctl(int fd, int request, void * data) {
  static ioctl_t real_ioctl = 0;

  BUILD_BUG_ON(sizeof(vgpucfg_t) != 0x730);

  if (!real_ioctl)
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");

  int ret = real_ioctl(fd, request, data);

  // Not a call we care about.
  if ((uint32_t) request != REQ_QUERY_GPU) return ret;

//  if (data != NULL)
//    syslog(LOG_INFO, "ioctl_request-> op is 0x%x\n", ((iodata_t *)data)->op_type);

  // Call failed
  if (ret < 0) return ret;

  iodata_t * iodata = (iodata_t * ) data;

  //Driver will try again
  if (iodata -> status == STATUS_TRY_AGAIN) return ret;

  if (iodata -> op_type == OP_READ_PCI_ID) {
    // Lookup address of the device and subsystem IDs.
    uint16_t * devid_ptr = (uint16_t * ) iodata -> result + 1;
    uint16_t * subsysid_ptr = (uint16_t * ) iodata -> result + 3;
    // Now we replace the device ID with a spoofed value that needs to
    // be determined such that the spoofed value represents a GPU with
    // vGPU support that uses the same GPU chip as our actual GPU.
    uint16_t actual_devid = * devid_ptr;
    uint16_t spoofed_devid = actual_devid;
    uint16_t actual_subsysid = * subsysid_ptr;
    uint16_t spoofed_subsysid = actual_subsysid;

    // Maxwell
    if (0x1340 <= actual_devid && actual_devid <= 0x13bd ||
      0x174d <= actual_devid && actual_devid <= 0x179c) {
      spoofed_devid = 0x13bd; // Tesla M10
      spoofed_subsysid = 0x1160;
    }
    // Maxwell 2.0
    if (0x13c0 <= actual_devid && actual_devid <= 0x1436 ||
      0x1617 <= actual_devid && actual_devid <= 0x1667 ||
      0x17c2 <= actual_devid && actual_devid <= 0x17fd) {
      spoofed_devid = 0x13f2; // Tesla M60
    }
    // Pascal
    if (0x15f0 <= actual_devid && actual_devid <= 0x15f1 ||
      0x1b00 <= actual_devid && actual_devid <= 0x1d56 ||
      0x1725 <= actual_devid && actual_devid <= 0x172f) {
      spoofed_devid = 0x1b38; // Tesla P40
    }
    // GV100 Volta
    if (actual_devid == 0x1d81 || // TITAN V
      actual_devid == 0x1dba) { // Quadro GV100 32GB
      spoofed_devid = 0x1db6; // Tesla V100 32GB PCIE
    }
    // Turing
    if (0x1e02 <= actual_devid && actual_devid <= 0x1ff9 ||
      0x2182 <= actual_devid && actual_devid <= 0x21d1) {
      spoofed_devid = 0x1e30; // Quadro RTX 6000
      spoofed_subsysid = 0x12ba;
    }
    // Ampere
    if (0x2200 <= actual_devid && actual_devid <= 0x2600) {
      spoofed_devid = 0x2230; // RTX A6000
    }
    * devid_ptr = spoofed_devid;
    * subsysid_ptr = spoofed_subsysid;
  }

  if (iodata -> op_type == OP_READ_DEV_TYPE) {
    // Set device type to vGPU capable.
    uint64_t * dev_type_ptr = (uint64_t * ) iodata -> result;
    * dev_type_ptr = DEV_TYPE_VGPU_CAPABLE;
  }

  if (iodata->op_type == OP_READ_VGPUCFG) {
    vgpucfg_t *cfg = iodata->result;
    if (strncmp (cfg->vgpu_type, "NVS",3) == 0) {
        syslog(LOG_INFO, "vgpucfg: patching vgpu nvidia-%d profile\n", cfg->gpu_type);
        strncpy(cfg->features, "GRID-vGaming,8.0", sizeof(cfg->features));
        strncpy(cfg->maybe_used_feature, "GRID vGaming", sizeof(cfg->maybe_used_feature));
        cfg->display_width = 1920;
        cfg->display_height = 1080;
        cfg->max_pixels = 4147200;
        cfg->cuda_enabled = 1;
        cfg->frl_enabled = 0;
    }

    syslog(LOG_INFO, "vgpucfg gpu_type: %d\n", cfg->gpu_type);
    syslog(LOG_INFO, "vgpucfg card_name: %s\n", cfg->card_name);
    syslog(LOG_INFO, "vgpucfg vgpu_type: %s\n", cfg->vgpu_type);
    syslog(LOG_INFO, "vgpucfg features: %s\n", cfg->features);
    syslog(LOG_INFO, "vgpucfg max_instances: %d\n", cfg->max_instances);
    syslog(LOG_INFO, "vgpucfg num_displays: %d\n", cfg->num_displays);
    syslog(LOG_INFO, "vgpucfg display_width: %d\n", cfg->display_width);
    syslog(LOG_INFO, "vgpucfg display_height: %d\n", cfg->display_height);
    syslog(LOG_INFO, "vgpucfg max_pixels: %d\n", cfg->max_pixels);
    syslog(LOG_INFO, "vgpucfg frl_config: 0x%02x\n", cfg->frl_config);
    syslog(LOG_INFO, "vgpucfg cuda_enabled: %d\n", cfg->cuda_enabled);
    syslog(LOG_INFO, "vgpucfg ecc_supported: %d\n", cfg->ecc_supported);
    syslog(LOG_INFO, "vgpucfg mig_instance_size: %d\n", cfg->mig_instance_size);
    syslog(LOG_INFO, "vgpucfg multi_vgpu_supported: %d\n", cfg->multi_vgpu_supported);
    syslog(LOG_INFO, "vgpucfg pci_id: 0x%08lx\n", cfg->pci_id);
    syslog(LOG_INFO, "vgpucfg pci_device_id: 0x%08lx\n", cfg->pci_device_id);
    syslog(LOG_INFO, "vgpucfg framebuffer: 0x%08lx\n", cfg->framebuffer);
    syslog(LOG_INFO, "vgpucfg mappable_video_size: 0x%08lx\n", cfg->mappable_video_size);
    syslog(LOG_INFO, "vgpucfg framebuffer_reservation: 0x%08lx\n", cfg->framebuffer_reservation);
    syslog(LOG_INFO, "vgpucfg encoder_capacity: 0x%02lx\n", cfg->encoder_capacity);
    syslog(LOG_INFO, "vgpucfg bar1_length: 0x%02lx\n", cfg->bar1_length);
    syslog(LOG_INFO, "vgpucfg frl_enabled: %d\n", cfg->frl_enabled);
    syslog(LOG_INFO, "vgpucfg maybe_used_feature: %s\n", cfg->maybe_used_feature);
  }

  if(iodata->status != STATUS_OK) {
    // Things seems to work fine even if some operations that fail
    // result in failed assertions. So here we change the status
    // value for these cases to cleanup the logs for nvidia-vgpu-mgr.
    if(iodata->op_type == 0xA0820104 ||
      iodata->op_type == 0x90960103 ||
      iodata->op_type == 0x90960101) {
        iodata->status = STATUS_OK;
        } else {
          syslog(LOG_ERR, "op_type: 0x%08X failed, status 0x%X.", iodata->op_type, iodata->status);
      }
  }

  // Workaround for some Maxwell cards not supporting reading inforom.
  if (iodata -> op_type == 0x2080014b && iodata -> status == 0x56) {
    iodata -> status = 0x57;
  }

  return ret;
}
