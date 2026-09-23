```
88888ba   888 888  888  888 8888888bad88ba
888 "888  888 888  888  888 888  "888  "88b
888  888  888 888  888  888 888   888   888
888  Y88b 888 Y88b 888 d88P 888   888   888
888   "Y88888  "Y8888888P"  888   888   888
```

# nuwm (Not Micro Window Manager)

**nuwm** adalah pengelola jendela (*window manager*) sederhana untuk X11 yang dibangun menggunakan library XCB.

**nuwm** hanya menyediakan perintah-perintah primitif yang umum digunakan. Pola penggunaan muncul langsung dari interaksi pengguna dengan **nuwm**. Tidak ada status bar, widget, atau dekorasi di layar. **nuwm** hanya menyediakan perintah pengatur jendela tanpa mengatur layout secara otomatis.

Tidak ada *workspace* khusus. Hanya ada **9 slot client** yang tersedia. **nuwm** hanya menyimpan dua state fokus di dalam memori:

- `focus_curr`: Fokus jendela saat ini.
- `focus_prev`: Fokus jendela sebelumnya.

## Spesifikasi Teknis (Glibc)

- **Source Code:** ~200 LOC (Single `.c` file)
- **Binary Size:** ~6.5 KB
- **RAM Footprint:** ~2 MB Resident (RES) / ~150 KB Private (PRIV)

## Requirements

Sebelum mengompilasi **nuwm**, pastikan dependensi berikut sudah terinstall di sistem Anda:

- `libxcb`
- `gcc`
- `make`

## Cara Instalasi, Menjalankan & Uninstall

### Install

```bash
git clone https://github.com/Rhevalk/nuwm.git
cd nuwm
sudo make clean install
```

### Menjalankan

```bash
#~/.xinitrc
exec nuwm
```

### Uninstall

```bash
sudo make clean uninstall
```

## Struktur Komunikasi IPC

**nuwm** berkomunikasi menggunakan dua file di RAM (`tmpfs`) dan satu skrip kontrol di `$HOME`:

- `/dev/shm/nuwm.in` → Pipe FIFO untuk menerima input perintah.
- `/dev/shm/nuwm.out` → File biasa untuk menulis status/state `nuwm`.
- `~/.nuwmrc` → Skrip Shell yang dieksekusi saat tombol `Super_L` ditekan.

## Interaksi IPC

### Mengirim Perintah ke `nuwm.in`

Karena `nuwm.in` berbasis FIFO, Anda cukup mengalirkan string via `echo`:

```bash
echo "command" > /dev/shm/nuwm.in
```

### Membaca Status dari `nuwm.out`

Anda bisa menggunakan `read` atau `cat` untuk membaca data disana:

```bash
read client < /dev/shm/nuwm.out
# atau
cat /dev/shm/nuwm.out
```

## Format Pembacaan `nuwm.out`

`nuwm.out` digunakan untuk membaca state dari `nuwm` (`focus_curr`, `focus_prev`, dan `client_list`).

Format penulisan string (dibaca dari kiri):

1. **Karakter 1:** Slot client yang fokus saat ini (`focus_curr`).
2. **Karakter 2:** Slot client yang fokus sebelumnya (`focus_prev`).
3. **Karakter 3 dan seterusnya:** Daftar slot client yang aktif (`client_list`).

**Contoh State:**

- `00` → Tidak ada client (`focus_curr` dan `focus_prev` bernilai 0).
- `101` → Ada client di slot 1; `focus_curr` mengarah ke slot 1.
- `1212` → Ada client di slot 1 dan 2; `focus_curr` = slot 1, `focus_prev` = slot 2.

## File Kontrol (`~/.nuwmrc`)

`~/.nuwmrc` adalah file yang dieksekusi **nuwm** untuk memanggil perintah shell dari luar. Anda bisa memanfaatkannya untuk memanggil bar atau launcher seperti `nuin`, `dmenu` / `rofi`.

Buat file kontrol tanpa perlu menambahkan izin eksekusi (`chmod +x` tidak diperlukan):

```bash
touch ~/.nuwmrc
```

## Keybind Tunggal

**nuwm** sengaja dibangun dengan penggunaan CLI, tetapi **nuwm** tetap mendaftarkan **satu keybind utama** di tingkat sistem:

| Keybind | Aksi |
| --- | --- |
| `Super_L` (Tombol Mod4) | Mengeksekusi skrip `~/.nuwmrc` via shell |

Dengan Keybind inilah `~/.nuwmrc`  dieksekusi yang nantinya bisa digunakan untuk menajalankan script yang ada disana.

## Mekanisme Parser Perintah

Perintah dikirimkan melalui IPC FIFO (`/dev/shm/nuwm.in`). Parser nuwm menggunakan sistem **Zero-Space Parsing** yang fleksibel, di mana perintah, target, dan argumen dapat ditulis dengan atau tanpa spasi.

> **Catatan:** Buffer input internal dibatasi maksimal **32 karakter** per satu kali kiriman string.
> 

### Format Umum

```
<command>[target][argumen]
```

### Referensi Perintah

| Command | Fungsi | Contoh | Detail / Catatan |
| --- | --- | --- | --- |
| **`q`** | Keluar dari nuwm | `q` | Menghentikan proses window manager |
| **`:`** | Eksekusi perintah Bash | `:nutm` | Menjalankan sisa string perintah ke `/bin/bash` |
| **`s`** | Tukar posisi (*swap*) slot | `s12` | Menukar slot client 1 dan slot client 2 |
| **`f`** | Fokus & tampilkan client | `f1` | Memindahkan fokus dan menaikkan jendela ke paling atas |
| **`g`** | Atur geometri (*geometry*) | `g1 w960 h1080 x0 y0` | Mengubah posisi/ukuran (`x`, `y`, `w`, `h`) |
| **`k`** | Bunuh client (*force kill*) | `k` atau `k12` | Menutup paksa client |
| **`m`** | Sembunyikan client (*unmap*) | `m` atau `m1` | Menyembunyikan jendela tanpa menghentikan prosesnya |

> **Catatan:** Perintah `k` memanggil `xcb_kill_client` yang akan menutup koneksi X11 client secara paksa.
> 

### Fitur Parser

#### **1. Target Implisit & Relatif (0–9)**

- **Tanpa target ditulis:** Otomatis mengarah ke client yang sedang aktif (`focus_curr`).
- **Target `1`–`9`:** Mengarah ke slot client ke-1 hingga ke-9 (`client_list[0..8]`).
- **Target `0`:** Mengarah ke client yang aktif sebelumnya (`focus_prev`).
- *Contoh:* `k` (bunuh client aktif), `k0` (bunuh client sebelumnya), `f0` (kembali fokus ke client sebelumnya).

#### **2. Pengabaian Spasi (*Zero Spacing*)**

Spasi bersifat opsional. Parser secara otomatis memisahkan huruf perintah, target slot, dan nilai argumen.

- *Dengan spasi:* `g1 w960 x0`
- *Tanpa spasi:* `g1w960x0`

#### **3. Multi-Targeting**

Eksekusi satu perintah ke beberapa slot secara berurutan dalam satu pemanggilan.

- `f12` $\rightarrow$ Tampilkan dan berikan fokus pada client 1, lalu client 2.
- `k12` $\rightarrow$ Bunuh client di slot 1 dan slot 2 secara berurutan.

#### **4. Rangkaian Perintah (*Command Chaining*)**

Beberapa perintah berbeda dapat digabungkan dalam satu string buffer tanpa perlu pemisah khusus.

- `k12f3` $\rightarrow$ Bunuh client 1 dan 2 (`k12`), lalu fokus ke client 3 (`f3`).
- `k:nutm` $\rightarrow$ Bunuh client saat ini (`k`), lalu jalankan perintah terminal (`:nutm`).

## Penggunaan Lanjutan

### 1. Emulasi Workspace (Non-Eksplisit)

Karena **nuwm** tidak memiliki fitur *workspace*, Anda bisa memanfaatkan multi-target. Jika client 1 dan client 2 diset sebagai split, memanggil `f12` akan menampilkan keduanya secara bersamaan sehingga bertindak layaknya satu *workspace*.

### 2. Pengaturan Geometri

Parameter geometri bersifat opsional (`w`, `h`, `x`, `y`). Nilai yang tidak ditulis tidak akan diubah.

#### Rumus Dasar Split & Ukuran Layar

Untuk menentukan lebar (*width*) dan posisi (*x-offset*) secara dinamis berdasarkan lebar layar Anda ($W_{screen}$):

$$
\text{Lebar Split (Half Width)} = \frac{W_{screen}}{2}
$$

$$
\text{Posisi Kanan (Right Offset)} = \frac{W_{screen}}{2}
$$

Contoh pada layar **Full HD ($1920 \times 1080$)**:

- $W_{screen} = 1920$
- Lebar Split Kiri/Kanan = $\frac{1920}{2} = 960$
- Posisi $X$ Kanan = $960$

**Contoh Perintah Geometri:**

- **Fullscreen:** `g w1920 h1080 x0 y0`
- **Split Kiri (Lebar 960, Posisi X=0):** `g w960 x0`
- **Split Kanan (Lebar 960, Posisi X=960):** `g w960 x960`
- **Layout 2 Client Split Vertikal Sekaligus:** `g12 w960 g2 x960`

### 3. Dukungan Multi-Monitor

**nuwm** secara teknis dapat digunakan pada setup *multi-monitor*. Karena `nuwm` tidak mengelola tata letak layar secara otomatis, pemetaan jendela antar-layar diserahkan sepenuhnya kepada pengguna melalui kalkulasi koordinat `X` dan `Y` dan ukuran jendela `W` dan `H` pada perintah geometris.

**Contoh:** Jika Anda memiliki Monitor 1 (1920x1080) dan Monitor 2 (1920x1080) di sebelah kanan, Anda cukup mengirim perintah `g1 x1920 y0 w1920 h1080` untuk memindahkan jendela ke monitor kedua.

> **Catatan:** Tetap periksa kembali `xrandr` anda, disana terdapat informasi mengenai ukuran dan posisi monitor
> 


## Integrasi Status Bar & Input Launcher

> **Repository Terkait:** [nuin](https://github.com/Rhevalk/nuin)

**nuwm** tidak memiliki status bar bawaan untuk menjaga ukuran biner dan penggunaan RAM tetap kecil. Sebagai pasangannya, Anda dapat menggunakan **nuin** sebuah bar dan *input launcher* minimalis yang dirancang khusus untuk bekerja secara *native* dengan **nuwm** melalui skrip `~/.nuwmrc`.

`nuin` berfungsi ganda:

- **Status Display:** Menampilkan indikator slot client yang aktif, jam/tanggal, serta persentase baterai.
- **Interactive Launcher:** Mengirimkan *command* atau perintah yang Anda ketik langsung ke FIFO Pipe `/dev/shm/nuwm.in`.

### Contoh Skrip `~/.nuwmrc`

Skrip ini dieksekusi setiap kali tombol `Super_L` ditekan:

```bash
#!/bin/sh

read cap_str < /sys/class/power_supply/BAT0/capacity
read clients < /dev/shm/nuwm.out

now=$(date '+%a, %d %b|%H:%M')
date_str="${now%|*}"
time_str="${now#*|}"

FG="#FDF7E1"
BG="#1D2324"
CC="#2A3334"
CB="#394547"
CA="#E15443"

if [ -n "$clients" ]; then
    curr="${clients%"${clients#?}"}"
    rest="${clients#?}"
    prev="${rest%"${rest#?}"}"
    slots="${rest#?}"

    i=1
    while [ "$i" -le 9 ]; do
        case "$curr$prev$slots" in
            *"$i"*)
                if [ "$i" = "$curr" ]; then
                    bg=$CA
                elif [ "$i" = "$prev" ]; then
                    bg=$CB
                else
                    bg=$CC
                fi
                set -- "$@" -b "$i" "$FG" "$bg"
                ;;
        esac
        i=$((i + 1))
    done
fi

exec nuin -f "monospace:size=13" \
    "$@" \
    -b "[]=" "$FG" "$CC" \
    -e "$FG" "$BG" \
    -d "$date_str" "$FG" "$CC" \
    -d "$time_str" "$FG" "$CB" \
    -d "$cap_str%" "$FG" "$CA" \
    > /dev/shm/nuwm.in
```

> **Catatan:** Anda dapat menyesuaikan skrip di atas, khususnya pada jalur pembacaan baterai (`sysfs`). Anda juga bisa menambahkan informasi sistem lainnya seperti penggunaan RAM, CPU load, atau indikator WiFi.
>
