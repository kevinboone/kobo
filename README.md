<h1>kobo</h1>

Version 0.0.1, March 2017

<table align="center" width="30%" border="1" cellpadding="5">
<tr>
<td>
Please note that this utility does not yet work properly with the 
Kobo Forma -- the name of the mounted volume is subtly different to 
that in older devices. Adding support for the Forma is on my to-do list,
but it's a very long list, and it's not yet even in the top half. 
I might promote it to a more prominent position if there is any obvious
demand, so feel free to get in touch.
</td>
</tr>
</table>


<h2>What is this?</h2>

<code>kobo</code> is a very simple utility for Linux, for adding 
e-books to, and 
removing them from, a Kobo e-reader. It works best (for reasons
that will be explained) with MOBI and EPUB files, but it can manipulate any
file type that the Kobo can handle, sometimes with a little help.
<p/>
Managing books at the filesystem level is pretty easy with the Kobo
devices, because they mount as ordinary USB storage devices, and present a
simple filesystem. Kobo devices are pretty flexible in their filesystem
layout, and in the simplest case a book can be installed simply by
dropping a file anywhere into the Kobo's mounted directory. What
the <code>kobo</code> utility will do that goes beyond this is:

<ul>
<li>Mount the Kobo device if it is not already mounted, and unmount
it afterwards (so it can be safely unplugged)</li>
<li>Display all the books on the device, with meta-data (in some cases),
regardless of directory structure</li>
<li>Maintain an "author/title" structure on the e-reader
 that is compatible with the 
way Calibre works</li>
<li>Clean up empty directories after deleting books</li>
</ul>

So, essentially, it's a slightly nicer way of managing the Kobo than
simply using Linux commands. <code>kobo</code> isn't intended to be a 
replacement for heavyweight applications like Calibre, but a faster way
of doing simple management tasks.
<p/>

<h2>Usage examples</h2>

<pre class="codeblock">
# Install an EPUB file, mounting the Kobo filesystem if necessary
kobo -i /path/to/my/book.epub

# Get a list of installed books, with index numbers
kobo -l

# Get a list of installed books, without trying to display meta-data
kobo -ln

# Delete book number 10 in the list 
kobo -r 10

# Install a plain text file, specifying the author since it can't be guessed 
kobo -i /path/to/my/book.txt --author "Bloggs, Fred"
</pre>


<h2>Prerequisites</h2>

<code>kobo</code> is designed to run on modern Linux systems. It uses
the <code>udisksctl</code> utility to control the mounting of remote
devices, typically on <code>/var/run/media</code>. If this auto-mounting
does not work, it can be over-ridden by specifying the mount directory 
with the <code>--dir</code> switch. However, the more it is necessary
to fiddle about like this, the less advantage this utility offers over
simply copying files using operating system commands.
<p/>
<code>kobo</code> expects to be able to find the standard <code>unzip</code>
utility, for unpacking EPUB archives. 
<p/>
Most importantly, <code>kobo</code> depends on the <code>libebookinfo</code>
library, by the same author. To build <code>kobo</code>, you'll need to 
build and install <a href="http://github.com/kevinboone/ebookinfo">ebookinfo</a> first.

<h2>Building</h2>

Ensure that 
<a href="http://github.com/kevinboone/ebookinfo">ebookinfo</a> is built and installed.
Then proceed as usual:

<pre class="codeblock">
$ make
$ sudo make install
</pre>

<h2>Specifying an author</h2>

<code>kobo</code> tries to maintain a directory structure based on author
name on the device, in an attempt to be compatible with Calibre. With
MOBI and EPUB files, the utility can usually work out the author from the book
metadata; not so in other cases. If the author is unknown, use the
<code>--author</code> switch to provide one. Note that this switch does
nothing more than create a particular subdirectory on the e-reader's
storage; so far as Kobo is concerned, you can just use <code>--author /</code>
to put files in the top-level directory. This is a bit untidy, 
 however. 


<h2>Calibre issues</h2>

<code>kobo</code> does not interact with Calibre, or with any files that
Calibre creates. But it tries not to interfere with Calibre.
<p/>
It should go without saying that the <code>kobo</code> utility can't
send information to the Kobo device that is known only to Calibre. 
Calibre maintains a separate database of meta-data (author, series, genre)
that is indepent of anything stored in the book itself, and it is this internal
data Calibre uses when it writes a book to the device.
<p/>
This means that <code>kobo</code> can't send, for example, collection/bookshelf
information that Calibre displays, because this information is not 
stored in the book, but only in Calibre's database.  
<p/>
Note that modern Calibre versions do not scan the e-reader's stored files
when the device is connected -- they read only the stored meta-data on
the device. This means that changes made by the <code>kobo</code>
utility will not show up <i>until the device has been disconnected</i>
and the internal meta-data resynchronized.

<h2>File formats</h2>

At present, the only formats that <code>kobo</code> properly understands are 
EPUB and MOBI. When listing books on the device it will show author and title 
for these files; for everything else it will show only the file path.
Of course, if you store files on the device in author/title format, 
as Calibre does, then these displays might be very similar. Consequently,
you can specify the <code>--nometa</code> option to disable the 
extraction of meta-data. This will make listing files much quicker in 
large collections.
<p/>
Files other than EPUB or MOBI will need the <code>--author</code> switch when
copying to the device.

<h2>Notes</h2>

<code>kobo</code> will not examine directories on the Kobo device whose names begin with "." -- this is the convention for a hidden directory. Thus
it will not allow the user to delete the Kobo user guide, either accidentally
or on purpose
<p/>
No attempt is made to edit the stored meta-data on the Kobo device. 
The device will re-scan the files when it is disconnected. </li>
<p/>
It's possible, in principle, for a badly-formatted EPUB or MOBI file
to crash the program
<p/>
The default behaviour of <code>kobo</code>, if the reader is not
mounted, is to mount it, do whatever is specified, then unmount it.
This can be a bit tiresome in a system where mounting requires 
authentication. In such cases, bear in mind that <code>kobo</code> won't
unmount the e-reader if it finds it mounted; so it might be more convenient
to mount it using the operating system or desktop first. Bear in mind,
however, that Kobo devices are rather susceptible to misbehaving 
if they are unplugged
whilst still mounted

<h2>Bugs and limitations</h2>

When examining the list of mounted USB devices, <code>kobo</code> assumes
that the first mount it encountered with the device name that contains
the string "kobo" (regardless of letter case) is the one to use.
There is thus no obvious way to support multiple Kobo devices, apart from
the crudely obvious one.

<h2>Legal, etc</h2>

<code>kobo</code> is maintained by Kevin Boone, and distributed under the
terms of the GNU Public Licence, version 3.0. Essentially, you may do
whatever you like with it, provided the original author is acknowledged, and
you accept the risks involved in doing so. <code>kobo</code> contains
contributions from a number of other authors, whose details may be
found in the source code.
<p/>

