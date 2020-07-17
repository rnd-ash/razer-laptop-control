#include "chroma.h"

struct row_data matrix[5];


int displayMatrix(struct usb_device *usb) {
    int row;
    for (row=0; row<=5; row++) {
        sendRowDataToProfile(usb, row);
    }
    return 0;
}

int sendRowDataToProfile(struct usb_device *usb, int row_number) {
    char buffer[90];
    struct razer_packet packet;
    memset(buffer, 0x00, sizeof(buffer));
    memset(&packet, 0x00, sizeof(packet));
    packet.dev = 0x1f;
    packet.args_size = 0x34;
    packet.cmd_id = 0x03;
    packet.sub_cmd_id = 0x0b;

    packet.args[0] = 0xff;
    packet.args[1] = row_number;
    packet.args[3] = 0x0f;
    memcpy(&packet.args[7], &matrix[row_number].keys, 45);
    memcpy(buffer, &packet, 90);
    return send_payload(usb, buffer, 800, 1000);
}

int displayProfile(struct usb_device *usb, int profileNum) {
    char buffer[90];
    struct razer_packet packet;
    memset(buffer, 0x00, sizeof(buffer));
    memset(&packet, 0x00, sizeof(packet));

    packet.dev = 0x1f;
    packet.args_size = 0x02;
    packet.cmd_id = 0x03;
    packet.sub_cmd_id = 0x0a;

    packet.args[0] = 0x05;
    packet.args[1] = 0x00;
    memcpy(buffer, &packet, 90);
    return send_payload(usb, buffer, 1000, 2000);
}

int sendBrightness(struct usb_device *usb, __u8 brightness) {
    char buffer[90];
    struct razer_packet packet;
    memset(buffer, 0x00, sizeof(buffer));
    memset(&packet, 0x00, sizeof(packet));

    packet.dev = 0x1f;
    packet.args_size = 0x03;
    packet.cmd_id = 0x03;
    packet.sub_cmd_id = 0x03;

    packet.args[0] = 0x01;
    packet.args[1] = 0x05;
    packet.args[2] = brightness;
    memcpy(buffer, &packet, 90);
    return send_payload(usb, buffer, 1000, 2000);
}

int getBrightness(struct usb_device *usb) {
    char req[90];
    char resp[90];
    memset(resp, 0x00, sizeof(resp));
    memset(req, 0x00, sizeof(req));
    req[1] = 0x1f;
    req[5] = 0x02;
    req[6] = 0x0E;
    req[7] = 0x84;
    req[8] = 0x01;
    recv_payload(usb, req, resp, 800, 1000);
    return resp[9];
}
