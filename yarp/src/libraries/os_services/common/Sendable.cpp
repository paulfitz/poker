
#include "Sendable.h"
#include "Sendables.h"

//#include "Image.h"
#include "RefCounted.h"
//#include "Port.h"


int Sendable::Destroy()
{
  int destroyed = 0;
  if (owner!=NULL)
    {
      owner->TakeBack(this);
      destroyed = 0;
    }
  else
    {
      cout << ">>> Sendable::Destroy() killed " << ((long int) (this)) << endl;
      delete this;
      destroyed = 1;
    }
  return destroyed;
}



/*
template <class T>
class SimplePortable
{
public:
  T datum;

  SendablesOf<SimpleSendable<T> > sendables;

  void Fire(Port& port)
    {
      SimpleSendable<T> *sendable = sendables.Get();
      if (sendable == NULL) sendable = new SimpleSendable<T>;
      assert(sendable!=NULL);
      sendable->owner = &sendables;
      sendable->datum = datum;
      //port.Give(sendable);
    }

  void PermitModification()
    {
    }

  void PermitOverwrite()
    {
    }
};

SimplePortable<int> x;
*/

/*
class ImageSendable : public Sendable
{
public:
  struct Header
  {
    int len;
    int w;
    int h;
    int depth;
  } header;
  char *buffer;
  int is_owner;

  ImageSendable()
    {
      buffer = NULL;
      is_owner = 0;
    }

  virtual ~ImageSendable()
    {
      printf("Imagesendable DESTRUCTOR\n");
      if (is_owner&&buffer!=NULL)
	{
	  delete[] buffer;
	  buffer = NULL;
	}
    }

  virtual int Fire(BlockSender& sender)
    {
      assert(buffer!=NULL);
      sender.Add((char*)(&header),sizeof(header));
      sender.Add(buffer,header.len);
      return 1;
    }
};
*/

/*
class ImagePortable : public GenericImage
{
public:

  SendablesOf<ImageSendable > sendables;
  ImageSendable *last_sendable;

  ImagePortable() 
    { last_sendable = NULL; }

  virtual ~ImagePortable()
    { if (last_sendable!=NULL) last_sendable->RemoveRef(); }

  void Fire(Port& port)
    {
      ImageSendable *sendable = sendables.Get();
      if (sendable == NULL) sendable = new ImageSendable;
      // set target to sendables HIT
      assert(sendable!=NULL);
      sendable->owner = &sendables;
      sendable->header.h = GetHeight();
      sendable->header.w = GetWidth();
      //port.Give(sendable);
    }

  void PermitModification()
    {
    }

  void PermitOverwrite()
    {
    }
};

ImagePortable y;

*/


/*
template <class T>
class IFragment
{
public:
  void SwitchFragments(T& resource);
  void CopyFragments(T& resource);
};

class Sendable
{
public:
  virtual void Fire(BlockSender& sender) {}
  virtual ~SendablePackage() {}
  virtual void Release() {}
};

template <class T>
class SendablePackageOf
{
public:
  T datum;
  virtual void Send(BlockSender& sender)
    {
      sender.Add((char*)(&datum),sizeof(datum));
    }
};

class SendablePackageOfImage
{
public:
  struct Header
  {
    int w;
    int h;
  } header;
  virtual void Send(BlockSender& sender)
    {
      sender.Add((char*)(&header),sizeof(header));
      
    }
};

class Sendable
{
public:
  Sema sender_count;

  Sendable() : sender_count(0)
    {}
};
*/

/*
void main()
{
}
*/
