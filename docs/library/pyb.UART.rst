.. _pyb.UART:

class UART -- duplex serial communication bus
=============================================

UART implements the standard UART/USART duplex serial communications protocol.  At
the physical level it consists of 2 lines: RX and TX.  The unit of communication
is a character (not to be confused with a string character) which can be 8 or 9
bits wide.

UART objects can be created and initialised using::

    from pyb import UART

    uart = UART(1, 9600)                         # init with given baudrate
    uart.init(9600, bits=8, parity=None, stop=1) # init with given parameters

Bits can be 7, 8 or 9.  Parity can be None, 0 (even) or 1 (odd).  Stop can be 1 or 2.

*Note:* with parity=None, only 8 and 9 bits are supported.  With parity enabled,
only 7 and 8 bits are supported.

A UART object acts like a stream object and reading and writing is done
using the standard stream methods::

    uart.read(10)       # read 10 characters, returns a bytes object
    uart.readall()      # read all available characters
    uart.readline()     # read a line
    uart.readinto(buf)  # read and store into the given buffer
    uart.write('abc')   # write the 3 characters

Individual characters can be read/written using::

    uart.readchar()     # read 1 character and returns it as an integer
    uart.writechar(42)  # write 1 character

To check if there is anything to be read, use::

    uart.any()               # returns True if any characters waiting

*Note:* The stream functions ``read``, ``write`` etc Are new in Micro Python since v1.3.4.
Earlier versions use ``uart.send`` and ``uart.recv``.

Constructors
------------

.. class:: pyb.UART(bus, ...)

   Construct a UART object on the given bus.  ``bus`` can be 1-6, or 'XA', 'XB', 'YA', or 'YB'.
   With no additional parameters, the UART object is created but not
   initialised (it has the settings from the last initialisation of
   the bus, if any).  If extra arguments are given, the bus is initialised.
   See ``init`` for parameters of initialisation.

   The physical pins of the UART busses are:

     - ``UART(4)`` is on ``XA``: ``(TX, RX) = (X1, X2) = (PA0, PA1)``
     - ``UART(1)`` is on ``XB``: ``(TX, RX) = (X9, X10) = (PB6, PB7)``
     - ``UART(6)`` is on ``YA``: ``(TX, RX) = (Y1, Y2) = (PC6, PC7)``
     - ``UART(3)`` is on ``YB``: ``(TX, RX) = (Y9, Y10) = (PB10, PB11)``
     - ``UART(2)`` is on: ``(TX, RX) = (X3, X4) = (PA2, PA3)``

Methods
-------

.. method:: uart.init(baudrate, bits=8, parity=None, stop=1, \*, timeout=1000, timeout_char=0, read_buf_len=64)

   Initialise the UART bus with the given parameters:

     - ``baudrate`` is the clock rate.
     - ``bits`` is the number of bits per character, 7, 8 or 9.
     - ``parity`` is the parity, ``None``, 0 (even) or 1 (odd).
     - ``stop`` is the number of stop bits, 1 or 2.
     - ``timeout`` is the timeout in milliseconds to wait for the first character.
     - ``timeout_char`` is the timeout in milliseconds to wait between characters.
     - ``read_buf_len`` is the character length of the read buffer (0 to disable).

   *Note:* with parity=None, only 8 and 9 bits are supported.  With parity enabled,
   only 7 and 8 bits are supported.

.. method:: uart.deinit()

   Turn off the UART bus.

.. method:: uart.any()

   Return ``True`` if any characters waiting, else ``False``.

.. method:: uart.read([nbytes])

   Read characters.  If ``nbytes`` is specified then read at most that many bytes.

   *Note:* for 9 bit characters each character takes two bytes, ``nbytes`` must
   be even, and the number of characters is ``nbytes/2``.

   Return value: a bytes object containing the bytes read in.  Returns ``b''``
   on timeout.

.. method:: uart.readall()

   Read as much data as possible.

   Return value: a bytes object.

.. method:: uart.readchar()

   Receive a single character on the bus.

   Return value: The character read, as an integer.  Returns -1 on timeout.

.. method:: uart.readinto(buf[, nbytes])

   Read bytes into the ``buf``.  If ``nbytes`` is specified then read at most
   that many bytes.  Otherwise, read at most ``len(buf)`` bytes.

   Return value: number of bytes read and stored into ``buf``.

.. method:: uart.readline()

   Read a line, ending in a newline character.

   Return value: the line read.

.. method:: uart.write(buf)

   Write the buffer of bytes to the bus.  If characters are 7 or 8 bits wide
   then each byte is one character.  If characters are 9 bits wide then two
   bytes are used for each character (little endian), and ``buf`` must contain
   an even number of bytes.

   Return value: number of bytes written.

.. method:: uart.writechar(char)

   Write a single character on the bus.  ``char`` is an integer to write.
   Return value: ``None``.
