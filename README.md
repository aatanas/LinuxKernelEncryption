# LinuxKernelEncryption

The goal of this project is to provide an encryption and decryption mechanism for Linux using a transposition cipher. The project is written in C and assembly within the Linux kernel.

The system supports the following functionalities:
- User can encrypt / decrypt files.
- Reading and writing using standard system calls works on encrypted files as well as normal files.
- The user can set the key used for encryption / decryption.
- The user can generate a random key.
- Dryptionirectory enc
   - Includes encryption of all files within the directory, as well as files in subdirectories.
- Temporary key caching, locally and globally. Cache is not inherited by child process.
- Key storage for encrypted files, to avoid file corruption using the wrong key.
- Key setting tool that doesn't print the key on the screen.

#### Cryptographic algorithm
For the purposes of encryption / decryption, a transposition algorithm based on
matrix column transposition is used, which is described [here](http://practicalcryptography.com/ciphers/columnar-transposition-cipher/).

The file is encrypted in blocks of 1024 bytes each (the entire buffer_head buffer which
is otherwise used for reading and writing files). A string of printable ASCII characters is used as the key
whose ASCII value is used for transposition. The length of the key will always be some
power of two and less than 1024, so a matrix with 1024 characters can certainly be
formed with that number of columns. The key will always be nonrepetitive.

The list of encrypted files is written to disk. This file / block is 
impossible to delete, overwrite, modify, etc. except for the normal operation of the encryption /
decryption system.

After the user sets the encryption key for the first time, it becomes possible to read and write
encrypted files. They can already be on the disk, or they can be ordinary files
encrypted using a special command. The functions read and write work normally and with
encrypted and non-encrypted files.

#### Tools
##### *keyset \<key>*
Sets the passed parameter as the currently active global key. The length of the key must
be a power of two and less than 1024. At the first call of this tool, the program
reads the list of encrypted files from the disk, and stores them in memory. After that, every call
to *read* or *write* functions over the encrypted file is successfully performed, i.e. the algorithm 
for encryption / decryption is applied when executing those calls.

##### *encr \<file>*
Encrypts the given file with the given key, in case it is not already encrypted. If the file
is already encrypted, an error is reported. This tool also records in the system that this file
is encrypted. If the key was not previously set using the keyset tool, an error is reported.

##### *decr \<file>*
Decrypts the given file using the given key, in case the file was previously encrypted. If
the file is not previously encrypted, an error is reported. This tool should also records in the system
that this file is no longer encrypted. If the key was not previously set using the keyset tool,
an error is reported.

##### *keyclear*
Resets the key to NULL, and disables further encryption and decryption until
*keyset* is called again. The *read* and *write* functions should again work as if they were unaware of
encryption.

##### *keygen level*
Generates a random key made up of printable ASCII characters and prints it to the screen.
The level parameter can be one of:
- 1 - a key of length 4 is generated;
- 2 - a key of length 8 is generated;
- 3 - a key of length 16 is generated.
If level is not given, or is not one of these three values, an error is reported.

#### Directory encryption / decryption
If the user makes a system call for encryption (using the encr tool) on a
directory, the following happens:
- All files inside the directory are encrypted, as well as the directory itself.
- The same operation is repeated for each subdirectory within the given directory.

This logic is built into the system call itself, not the *encr* tool.  
System call still doesn't work if the key is not set.

The tool and system call *decr* works the same way when called on a
directory.

#### Key caching for a limited time
The *keyset* system call can set a global or local key. Every
process has its own local key that it can set using this modified
*keyset* calls. the *keyset* tool still sets the global key.

For each process, the system remembers the last time it called *keyset* and if the specified time has passed
(default 45 sec.) the system automatically deletes the key for that process (as if it was
*keyclear* system call that runs locally).

The global key is emptied 2 min after initialisation.

The *keyclear* call clears either a global or a local key, while the *keyclear* tool clears the global one.

Forked processes do not inherit the parent's cached key.

#### Key storage for encrypted files
When encrypting a file, the hash value of the key with which it was encrypted is also recorded.
If an attempt is made to access a file (decr / read / write) with a wrong
key, the error code -EINVAL is returned instead of decryption.

When decrypting, the operation will only succeed on files
which are encrypted with the currently set key.

#### Modification of the keyset tool
When setting a key using the *keyset* tool, the key is no longer given as an argument to
command line. The tool is started with no arguments, and the user is expected to enter the key when
starting the tool. The characters that the user enters are not displayed on the screen while being
input.
