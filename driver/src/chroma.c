#include "chroma.h"

int sendRowDataToProfile(struct usb_device *usb, int row_number, char* row_bytes) {
    char buffer[90];
    struct razer_packet packet;
    int i;
    memset(buffer, 0x00, sizeof(buffer));
    memset(&packet, 0x00, sizeof(packet));
    packet.dev = 0x1f;
    packet.args_size = 0x34;
    packet.cmd_id = 0x03;
    packet.sub_cmd_id = 0x0b;

    packet.args[0] = 0xff;
    packet.args[1] = row_number;
    packet.args[3] = 0x0f;
    memcpy(&packet.args[7], &row_bytes[0], 45);
    memcpy(buffer, &packet, 90);
    return send_payload(usb, buffer, 1000, 2000);
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