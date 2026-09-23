workflow baru

nuwm tidak memiliki workspace, jadi ada trick khusus membuatnya seolah olah workspace
tapi anda tetap bisa mendapatkan rasa seperti workspace ketika 2 cllient atau lebih memiliki ukuran jendela yg berbeda, misal jika 2 jendela split, anda bisa memfokuksn kedua jendela itu untuk langsung menampilkan keduanya.

keungguna multicommand dan multitarget
f12 -> fokus 2 client sekaligus
r12 960 1080 m2 960 0 -> membuat 2 client split












xlsfonts | grep terminus | less
xset fp+ /usr/share/fonts/X11/misc/
xset fp rehash

cc -Os nuwm.c -o nuwm_debug -lX11 -lXft -lfontconfig -I/usr/include/freetype2
nm --size-sort -S --radix=d nuwm_debug | tail -n 20

sudo xbps-install -S xsetroot

perlu mengistall itu jika pake status bar




# nuwm

Window manager monolitik ultra-ringkas berbasis X11 untuk Linux.

---

## Prasyarat & Catatan Penting (Dependencies & Environment)

`nuwm` **tidak melakukan runtime error checking** pada inisialisasi resource (seperti alokasi memori, koneksi display, font, maupun windowing) demi meminimalkan ukuran biner. 

Jika ada syarat yang tidak terpenuhi, `nuwm` akan langsung mengalami **Segmentation Fault (Crash)** tanpa menampilkan pesan error.

Pastikan hal-hal berikut sudah terpenuhi sebelum menjalankan `nuwm`:

### 1. System Libraries (Build-time & Runtime)
- `libX11`
- `libXft` (secara otomatis menarik `libfontconfig` dan `freetype2` sebagai dependensi)

### 2. Runtime Environment & Font Setup
- **Single Window Manager:** Pastikan **TIDAK ADA Window Manager lain** (seperti `dwm`, `i3`, `openbox`, dll) atau Compositor mandiri yang sedang berjalan di *display* yang sama.
- **X11 Server Active:** X Server wajib sudah berjalan dan variabel `$DISPLAY` terkonfigurasi dengan benar (dipanggil dari `.xinitrc` atau via `startx`).
- **Valid Font System:** Font yang ditaruh pada `FONT_NAME` di `config.h` (default: `monospace:size=10`) **wajib ter-install dan terdaftar** di fontconfig.

Sebelum kompilasi, pastikan font sistem sudah ter-cache dengan benar:

```bash
# Perbarui cache font sistem
fc-cache -vf

# Cek apakah font sudah terdeteksi
fc-list : family | grep -i monospace













# Cut dari slot 1, lalu Paste ke slot 3 dalam 1 perintah
echo "x1 p3" > /tmp/uwm

# 1. Cut slot 1, paste ke slot 3
echo "x1 p3" > /tmp/uwm

# 2. Cut slot 2, paste ke slot 1
echo "x2 p1" > /tmp/uwm

# 3. Cut slot 3, paste ke slot 2
echo "x3 p2" > /tmp/uwm

arg 0 artinya prev focus (f0, m0, k0)
no arg artinya focus (f, m, k)



# uwm — micro window manager

```
88888b.   888 888  888  888 88888888b.d88b.  
888 "888  888 888  888  888 888  "888  "88b 
888  888  888 888  888  888 888   888   888 
888  Y88b 888 Y88b 888 d88P 888   888   888 
888   "Y88888  "Y8888888P"  888   888   888 
                                              
```

**uwm** (Micro Window Manager) adalah pengelola jendela extream minimalis untuk X11 yang dibangun menggunakan XCB.

Desain `uwm` berpusat pada sekumpulan operasi dasar yang dapat dikombinasikan secara langsung. Tata letak dan pola penggunaan muncul dari interaksi operasi-operasi tersebut, bukan dari aturan tata letak yang ditentukan sebelumnya.

---

## Filosofi

`uwm` tidak menyediakan bar, widget, mode layout otomatis, atau indikator status visual apa pun. Sistem ini memaksa Anda untuk menggunakan memori organik (otak) untuk mengingat status *workspace* dan jendela. Jika Anda merasa kelelahan, itu adalah indikator alami dari tubuh bahwa Anda perlu segera mengurangi beban kerja.

Karena tidak ada informasi visual mengenai *workspace* dan *client*, Anda perlu memastikan *launcher* pertama dapat bekerja untuk memunculkan aplikasi. Dari sanalah Anda dapat menguji dan merasakan bagaimana `uwm` sebenarnya bekerja.

---

## Konsep Pengelola Jendela

`uwm` tidak memiliki mode pengatur jendela statis (seperti *tiling/floating*), melainkan mengandalkan kumpulan operasi dasar yang dapat menghasilkan banyak perilaku tata letak tanpa perlu dideklarasikan secara eksplisit.

Secara fundamental, `uwm` melakukan *tiling* (memaksimalkan ukuran) pada setiap jendela baru, namun tidak meletakkannya berdampingan secara otomatis, melainkan langsung menumpuknya begitu saja. Sifat ini menciptakan perilaku *monocle* non-eksplisit secara instan. Anda cukup menggunakan fungsi `switch_focus` untuk berselancar di dalam tumpukan (*stack*) jendela tersebut.

Fitur unggulan `uwm` dalam manajemen jendela adalah **Fluid Grid Snapping**. Anda dapat menempatkan jendela secara *vertical split*, *horizontal split*, atau membaginya ke dalam kuadran 1/4 layar secara kumulatif. Mekanisme ini memberikan fleksibilitas *tiling* dinamis yang andal hingga **8 clients** per *workspace*, yang dapat dikombinasikan secara bebas dengan perilaku *monocle*.

---

## Arsitektur Inti

### Workspace Statis

Seluruh data *workspace* dan manajemen *client* disimpan menggunakan struktur data statis berukuran tetap (*fixed-size*). Tidak ada alokasi memori dinamis (`malloc/calloc`) apa pun saat jendela baru dibuat, yang ada hanyalah manipulasi bit murni untuk setiap entitas jendela.
Dengan pendekatan memori statis ini, data tidak perlu pernah dipindahkan atau diatur ulang di dalam memori. Alhasil, kompleksitas waktu dari hampir seluruh operasi di dalam `uwm` mendekati **O(1)** (Konstan).

```c
#define MAX_CLIENTS   8
#define MAX_WORKSPACE 4
```

---

## Sistem Snapping

Penempatan jendela dilakukan melalui sistem snapping berbasis arah.

Arah yang tersedia:

- Kiri
- Kanan
- Atas
- Bawah

Perintah snapping bersifat kumulatif. Setiap tindakan memodifikasi geometri jendela secara bertahap dan dapat diprediksi.

Melalui kombinasi operasi snapping, pengguna dapat membentuk berbagai susunan ruang kerja sesuai kebutuhan tanpa harus berpindah ke konfigurasi atau pengaturan lain.

### Mekanika Kumulatif

Karena `uwm` tidak memiliki mesin pengatur otomatis, Anda membentuk layout secara manual lewat urutan snapping yang logis pada jendela yang sedang fokus:

- **Membuat Vertical Split (2 Jendela Berdampingan):**
    1. Buka jendela pertama (otomatis berukuran penuh).
    2. Tekan `Mod + Left` (jendela pertama menyusut mengisi 1/2 layar kiri).
    3. Buka jendela kedua (otomatis menumpuk berukuran penuh).
    4. Tekan `Mod + Right` (jendela kedua menyusut mengisi 1/2 layar kanan).
- **Membuat Kuadran 1/4 Layar (Pojok Kanan Atas):**
    1. Pada jendela aktif, tekan `Mod + Right` (mengisi 1/2 kanan).
    2. Tekan `Mod + Up` (jendela otomatis menyusut lagi mengisi 1/4 kuadran kanan atas).
    3. Menekan arah berlawanan (`Mod + Down`) akan mengembalikan ukuran jendela ke setengah layar penuh terlebih dahulu sebelum berpindah posisi.

---

## Konfigurasi

Seluruh konfigurasi dilakukan melalui:

```c
config.h
```

Isi dari file `config.h` hanya mengatur Mod Key dan pintasan  untuk menjalankan fungsi fungsi yang di sediakan. Anda juga bisa memanggil aplikasi dari `uwm` dengan bantuan makro `SHCMD(cmd)`  atau dengan memanggilnya langsung jika anda sudah menetapkan alamatnya.

> ⚠️ **Penting:** Pastikan anda membuat pintasan untuk membuka Launcher dan perubahan konfigurasi memerlukan kompilasi ulang
> 

---

## Pintasan Bawaan

| Tombol | Aksi |
| --- | --- |
| Mod + Return | Membuka terminal |
| Mod + d | Membuka menu |
| Mod + Tab | Berpindah ke jendela berikutnya |
| Mod + Shift + q | Menutup jendela aktif |
| Mod + Shift + Escape | Keluar dari uwm |
| Mod + Left | Snap ke kiri |
| Mod + Right | Snap ke kanan |
| Mod + Up | Snap ke atas |
| Mod + Down | Snap ke bawah |
| Mod + 1~4 | Berpindah workspace |
| Mod + Shift + 1~4 | Mengirim jendela ke workspace tujuan |

---

## Instalasi

### Dependensi

Arch Linux:

```bash
sudo pacman -S libxcb xcb-util-keysyms
```

### Kompilasi

```bash
git clone https://github.com/Rhevalk/uwm.git
cd uwm

cp config.def.h config.h

make
sudo make install
```

## Menjalankan

Tambahkan pada `.xinitrc`:

```bash
exec uwm
```
