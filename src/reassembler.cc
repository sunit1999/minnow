#include "reassembler.hh"

using namespace std;

void Reassembler::check_contiguous() {
  while (buffer.find(first_unassembled_idx()) != buffer.end())
  {
    string ready_data = "";
    ready_data += buffer[first_unassembled_idx()];
    buffer.erase(first_unassembled_idx());
    _bytes_pending--;

    output_.writer().push(ready_data);
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;

  uint64_t last_index = first_index;
  if (data.size() > 0) {
    last_index += data.size() - 1;
  }

  // Ignore out of bound indexes
  if (first_index >= first_unacceptable_idx() or last_index < first_unassembled_idx())
  {
    return;
  }

  if (data.size() == 0) {
    if (is_last_substring == true) _eof = true;

    if (_eof and bytes_pending() == 0)
    {
      output_.writer().close();
    }
    
    return;
  }
  
  if (is_last_substring == true and last_index < first_unacceptable_idx())
  {
    _eof = true;
  }

  // Can push directly?
  if (first_index <= first_unassembled_idx() && first_unassembled_idx() <= last_index)
  {
    uint64_t skip_data_bytes = first_unassembled_idx() - first_index;
    string ready_data = "";
    for (uint64_t i = first_index + skip_data_bytes; i <= min(last_index, first_unacceptable_idx() - 1); i++)
    {
      ready_data += data[i-first_index];

      // Erase from buffer
      if (buffer.find(i) != buffer.end())
      {
        buffer.erase(i);
        _bytes_pending--;
      }
    }

    output_.writer().push(ready_data);
    check_contiguous();

    if (bytes_pending() == 0 and _eof == true)
    {
      output_.writer().close();
    }
    
    return;
  }
  
  // Store in buffer
  for (uint64_t i = first_index; i <= min(last_index, first_unacceptable_idx() - 1); i++)
  {
    if (buffer.find(i) == buffer.end()) {
      buffer[i] = data[i-first_index];
      _bytes_pending++;
    }
  }
  
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return _bytes_pending;
}
