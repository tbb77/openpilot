#include "messaging.hpp"
#include "common/util.h"
#include "common/swaglog.h"

#include "ublox_msg.h"
#include "kaitai/kaitaistream.h"

ExitHandler do_exit;
using namespace ublox;

int main() {
  LOGW("starting ubloxd");
  AlignedBuffer aligned_buf;
  UbloxMsgParser parser;

  Context * context = Context::create();
  SubSocket * subscriber = SubSocket::create(context, "ubloxRaw");
  assert(subscriber != NULL);
  subscriber->setTimeout(100);

  PubMaster pm({"ubloxGnss", "gpsLocationExternal"});

  while (!do_exit) {
    Message * msg = subscriber->receive();
    if (!msg){
      if (errno == EINTR) {
        do_exit = true;
      }
      continue;
    }

    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(msg));
    cereal::Event::Reader event = cmsg.getRoot<cereal::Event>();
    auto ubloxRaw = event.getUbloxRaw();

    const uint8_t *data = ubloxRaw.begin();
    size_t len = ubloxRaw.size();
    size_t bytes_consumed = 0;

    while(bytes_consumed < len && !do_exit) {
      size_t bytes_consumed_this_time = 0U;
      if(parser.add_data(data + bytes_consumed, (uint32_t)(len - bytes_consumed), bytes_consumed_this_time)) {

        try {
          auto msg = parser.gen_msg();
          if (msg.second.size() > 0) {
            auto bytes = msg.second.asBytes();
            pm.send(msg.first.c_str(), bytes.begin(), bytes.size());
          }
        } catch (const std::exception& e) {
          LOGE("Error parsing ublox message %s", e.what());
        }

        parser.reset();
      }
      bytes_consumed += bytes_consumed_this_time;
    }
    delete msg;
  }

  delete subscriber;
  delete context;

  return 0;
}
