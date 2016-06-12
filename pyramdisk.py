from io import BytesIO
SIZE = 100 * 1024 * 1024
ramdisk = BytesIO()
ramdisk.seek(SIZE - 1)
ramdisk.write(b'0')

def read(length, offset):
    print("Reading %s bytes at %s" % (length, offset));
    ramdisk.seek(offset)
    return ramdisk.read(length)

def write(data, offset):
    print("Writing %s bytes at %s" % (len(data), offset));
    ramdisk.seek(offset)
    ramdisk.write(data)

def disc():
    print("Disconnect")

def flush():
    print("Flush")

def trim(offset, length):
    print("Trim from % for length %s" % ( offset, length ))

def size():
    print("Reporting block size of %s bytes" % SIZE)
    return SIZE
