# 2.1 gigabyte/second
eXdupe is a fast file archiver that supports deduplication and differential backups.

It's easy to use. Let's backup the entire D drive on Windows:

`exdupe d:\ d.full`

Each day we can then create a new backup that only stores the changes compared to the initial backup:

`exdupe -D d:\ d.full d1.diff`<br>
`exdupe -D d:\ d.full d2.diff`<br>

It finds identical data blocks of 4 KB at byte grannularity positions across terabytes. Example of a backup of a Windows and a Linux virtual machine of 57,776 MB in total:
|               | exdupe 1.1.0 | zpaq64 | 7-Zip-flzma2 | restic | tar+zstd |
|---------------|--------------:|-------:|--------------:|-------:|---------:|
| **Size**          |     28,005 MB | 29,633 MB |     32,331 MB | 33,518 MB | 35,982 MB |
| **Time**          |          24 s |    366 s |         562 s |     72 s |     35 s |
| **Options**       |       -g1 -t10 | -m1 -t12 |       fastest | default |    -1 -T0 |

Try the latest development version (*backwards compatibility broken often!*) that doubles the speed and improves compression ratio: [Windows](https://github.com/rrrlasse/exdupe/raw/stuff/beta/exdupe21.exe) and [Linux amd64](https://github.com/rrrlasse/eXdupe/raw/stuff/beta/exdupe_1.1.0.dev21_linux_amd64.tar.gz).

Or get the [latest stable version](https://github.com/rrrlasse/eXdupe/releases/tag/v1.0.0).
<img src="https://github.com/rrrlasse/exdupe/blob/stuff/cmd.webp" width="80%">
