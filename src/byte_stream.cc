#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) 
  : capacity_(capacity), deque_(), bytes_pushed_(0), bytes_popped_(0) {
}

bool Writer::is_closed() const
{
  // Your code here.
  return eof_;
}

void Writer::push( string data )
{
  // Your code here.
  // (void)data;
  if (data.size() > available_capacity()) {
  }

  for(const char& c: data) {
    if (available_capacity() > 0) {
      deque_.push_back(c);
      bytes_pushed_++;
    } else break;
  }

  return;
}

void Writer::close()
{
  // Your code here.
  eof_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - deque_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return eof_ && deque_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  static string data;
  data.clear();

  for (const char& c : deque_) {
    data += c;
  }

  return string_view(data);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  // (void)len;
  while (!deque_.empty() and len>0) {
    deque_.pop_front();
    bytes_popped_++;
    len--;
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return deque_.size();
}
