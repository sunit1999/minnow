#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  (void)message;
  if (message.RST) {
    rst_recieved = true;
    const_cast<Writer&>(writer()).set_error();
    return;
  }

  if (syn_recieved and message.seqno == ISN) return;

  if (message.SYN)
  {
    ISN = message.seqno;
    syn_recieved = true;
  }

  if (syn_recieved and message.FIN) {
    fin_recieved = true;
  }
  
  uint64_t stream_idx = 0;
  uint64_t abs_seqno = message.seqno.unwrap(ISN, reassembler_.first_unassembled_idx());
  if (abs_seqno > 0) stream_idx = abs_seqno - 1;
  reassembler_.insert(stream_idx, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message = TCPReceiverMessage();
  if (syn_recieved)
  { 
    uint64_t ackno = reassembler_.first_unassembled_idx();

    if (reassembler_.bytes_pending() == 0 and fin_recieved) {
      ackno++;
    }

    message.ackno = Wrap32::wrap(ackno + 1, ISN);
  }
  
  message.window_size = UINT16_MAX;
  if (reassembler_.available_capacity() < UINT16_MAX) {
    message.window_size = reassembler_.available_capacity();
  }
  
  message.RST = writer().has_error();
  if (rst_recieved) {
    message.RST = true;
  }
  return message;
}
