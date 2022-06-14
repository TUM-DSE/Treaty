static __inline unsigned char
inb (unsigned short int __port)
{
  unsigned char _v;
  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (__port));
  return _v;
}

static __inline unsigned short int
inw (unsigned short int __port)
{
  unsigned short _v;
  __asm__ __volatile__ ("inw %w1,%0":"=a" (_v):"Nd" (__port));
  return _v;
}

static __inline unsigned int
inl (unsigned short int __port)
{
  unsigned int _v;
  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (__port));
  return _v;
}

	static __inline void
outb_p (unsigned char __value, unsigned short int __port)
{
  __asm__ __volatile__ ("outb %b0,%w1\noutb %%al,$0x80": :"a" (__value),
                        "Nd" (__port));
}

static __inline void
outw_p (unsigned short int __value, unsigned short int __port)
{
  __asm__ __volatile__ ("outw %w0,%w1\noutb %%al,$0x80": :"a" (__value),
                        "Nd" (__port));
}

static __inline void
outl_p (unsigned int __value, unsigned short int __port)
{
  __asm__ __volatile__ ("outl %0,%w1\noutb %%al,$0x80": :"a" (__value),
                        "Nd" (__port));
}

