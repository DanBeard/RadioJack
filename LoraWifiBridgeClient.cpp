#include "LoraWifiBridgeClient.h"
#include "Meshtastic.h" 
#include "pb_encode.h"
#include "pb_decode.h"

// set to false once we've debugged
#define LORA_DEBUG true

// default broadcast
#define LORA_DEST_ADDR 0xFFFFFFFF
// default first channel
#define LORA_CHANNEL_IDX 2

// constants taken from mt_protocol.cpp
// Magic number at the start of all MT packets
#define MT_MAGIC_0 0x94
#define MT_MAGIC_1 0xc3

// The header is the magic number plus a 16-bit payload-length field
#define MT_HEADER_SIZE 4

// The buffer used for protobuf encoding/decoding. Since there's only one, and it's global, we
// have to make sure we're only ever doing one encoding or decoding at a time.
#define PB_BUFSIZE 512

// Nonce to request only my nodeinfo and skip other nodes in the db
#define SPECIAL_NONCE 69420

// Wait this many msec if there's nothing new on the channel
#define NO_NEWS_PAUSE 25


pb_byte_t mesh_buf[PB_BUFSIZE+4];

size_t LoraWifiBridgeClient::write(const uint8_t *buf, size_t size) {

  // thanks to meshtastic-arduino for this code
  // construct proto struct
  meshtastic_MeshPacket meshPacket = meshtastic_MeshPacket_init_default;
  meshPacket.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
  meshPacket.id = random(0x7FFFFFFF);
  meshPacket.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  meshPacket.to = LORA_DEST_ADDR; // this should be filled in by the radio right?
  meshPacket.channel = LORA_CHANNEL_IDX;
  meshPacket.want_ack = true;
  meshPacket.decoded.payload.size = size;
  memcpy(meshPacket.decoded.payload.bytes, buf, size);

  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_default;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;
  toRadio.packet = meshPacket;

  // construct raw packet
  mesh_buf[0] = MT_MAGIC_0;
  mesh_buf[1] = MT_MAGIC_1;

  pb_ostream_t stream = pb_ostream_from_buffer(mesh_buf + 4, PB_BUFSIZE);
  bool status = pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio);
  if (!status) {
    return 0;
  }

  // Store the payload length in the header
  mesh_buf[2] = stream.bytes_written / 256;
  mesh_buf[3] = stream.bytes_written % 256;


  return WiFiClient::write((const uint8_t *)mesh_buf, 4 + stream.bytes_written);
}

char str_buf[PB_BUFSIZE]; // string buf to hold resp
String LoraWifiBridgeClient::readString() {
  if (!connected()) {
    write_to_screen("Not connected. Weird");
    return String("");
  }

  memset(mesh_buf,0,PB_BUFSIZE);

  unsigned char* buf = mesh_buf; // use meshbuf to build the proto obj
  
  size_t bytes_read = 0;
  bool start1_seen =false;
  bool start2_seen = false;
  uint16_t payload_len = 0;
  while (this->available()) {
    char c = this->read();
    // only start once we get the first header magic bytes
    if(!start1_seen){
      if(c == MT_MAGIC_0) start1_seen = true;
      else continue;
    } else if(!start2_seen) {
        if(c == MT_MAGIC_1) start2_seen = true;
        else continue;
    } else if(payload_len == 0 && bytes_read >= 4) { // get packet size
        payload_len = mesh_buf[2] << 8 | mesh_buf[3];
        if(payload_len > PB_BUFSIZE)  {
          debug("Impossible Size!");
          return String("");
        }
    }
    
    *buf++ = c;
    if (++bytes_read >= PB_BUFSIZE + 4) {
      // corrupt start over
      debug("Bad packet. Buffer overrun");
      return String("");
    } 
    if(payload_len != 0 && bytes_read >= payload_len + 4){
      break; // we're done with this packet, move on. 
    }
  }

  // at this point buf should contain a single good fromRadio packet (with the 4 byte header in front)
  meshtastic_FromRadio fromRadio = meshtastic_FromRadio_init_zero;

  // Decode the protobuf
  pb_istream_t stream = pb_istream_from_buffer(mesh_buf + 4, payload_len);
  bool status = pb_decode(&stream, meshtastic_FromRadio_fields, &fromRadio);
  if (!status) {
    write_to_screen("Decoding Failed");
    return String("");
  }

  
  switch (fromRadio.which_payload_variant) {
    case meshtastic_FromRadio_my_info_tag:
    case meshtastic_FromRadio_node_info_tag:
    case meshtastic_FromRadio_config_complete_id_tag:
    case meshtastic_FromRadio_rebooted_tag:
      return String(""); // we don't care about these packets

    case meshtastic_FromRadio_packet_tag: {
      meshtastic_MeshPacket *meshPacket = &fromRadio.packet;
      if (meshPacket->which_payload_variant == meshtastic_MeshPacket_decoded_tag 
            && meshPacket->decoded.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP
            && meshPacket->channel == LORA_CHANNEL_IDX) {

        //zero out
        memset(str_buf,0,PB_BUFSIZE);
        memcpy(str_buf, meshPacket->decoded.payload.bytes,meshPacket->decoded.payload.size);
        return String(str_buf);
      } 
      break;
    }
      
    default:
      //debug("unrecon payload variant");
      return String("");
  }

  return String("");
}

void LoraWifiBridgeClient::requestConfigInfo() {
  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_default;
  toRadio.which_payload_variant = meshtastic_ToRadio_want_config_id_tag;
  toRadio.want_config_id = SPECIAL_NONCE;

  // construct raw packet
  mesh_buf[0] = MT_MAGIC_0;
  mesh_buf[1] = MT_MAGIC_1;

  pb_ostream_t stream = pb_ostream_from_buffer(mesh_buf + 4, PB_BUFSIZE);
  bool status = pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio);
  if (!status) {
    return;
  }

  // Store the payload length in the header
  mesh_buf[2] = stream.bytes_written / 256;
  mesh_buf[3] = stream.bytes_written % 256;


 WiFiClient::write((const uint8_t *)mesh_buf, 4 + stream.bytes_written);
 write_to_screen("Info req sent");
}

void LoraWifiBridgeClient::debug(char * msg) {
  write_to_screen(msg);
  write((const uint8_t *)msg, strlen(msg));
}