#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
#include <sstream>
using namespace std;

#include <boost/scoped_array.hpp>
using namespace boost;

#include "mcsconfig.h"

#include "exceptclasses.h"

#include "socketio.h"

namespace qfe
{
namespace socketio
{
void readn(int fd, void* buf, const size_t wanted)
{
  size_t needed = wanted;
  size_t sofar = 0;
  char* p = static_cast<char*>(buf);
  ssize_t rrc = -1;
  pollfd fds[1];
  int en = 0;

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  while (wanted > sofar)
  {
    fds[0].revents = 0;
    poll(fds, 1, -1);
    errno = 0;
    rrc = read(fd, (p + sofar), needed);
    en = errno;

    if (rrc < 0)
    {
      if (en == EAGAIN || en == EINTR || en == 512)
        continue;

      ostringstream oss;
      oss << "qfe: readn: read() returned " << rrc << " (" << strerror(en) << ")";
      idbassert_s(0, oss.str());
    }

    needed -= rrc;
    sofar += rrc;
  }
}

size_t writen(int fd, const void* data, const size_t nbytes)
{
  size_t nleft;
  ssize_t nwritten;
  const char* bufp = static_cast<const char*>(data);
  nleft = nbytes;

  while (nleft > 0)
  {
    // the O_NONBLOCK flag is not set, this is a blocking I/O.
    if ((nwritten = ::write(fd, bufp, nleft)) < 0)
    {
      if (errno == EINTR)
        nwritten = 0;
      else
      {
        // save the error no first
        int e = errno;
        string errorMsg = "qfe: writen: write() error: ";
        scoped_array<char> buf(new char[80]);
#if STRERROR_R_CHAR_P
        const char* p;

        if ((p = strerror_r(e, buf.get(), 80)) != 0)
          errorMsg += p;

#else
        int p;

        if ((p = strerror_r(e, buf.get(), 80)) == 0)
          errorMsg += buf.get();

#endif
        idbassert_s(0, errorMsg);
      }
    }

    nleft -= nwritten;
    bufp += nwritten;
  }

  return nbytes;
}

uint32_t readNumber32(SockType fd)
{
  uint32_t np;
  SockReadFcn(fd, &np, 4);
  return np;
}

string readString(SockType fd)
{
  string s;
  uint32_t len = readNumber32(fd);

  if (len > 0)
  {
    scoped_array<char> buf(new char[len + 1]);
    SockReadFcn(fd, buf.get(), len);
    buf[len] = 0;
    s = buf.get();
  }

  return s;
}

void writeString(SockType fd, const string& data)
{
  uint32_t len = data.length();
  SockWriteFcn(fd, &len, 4);
  SockWriteFcn(fd, data.c_str(), len);
}

}  // namespace socketio
}  // namespace qfe
