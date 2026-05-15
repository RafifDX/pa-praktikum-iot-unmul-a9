# PA Praktikum IoT UNMUL A9

## Nama dan NIM Anggota Kelompok

- **2209106023** — Muhammad Ihsan *(Active)*
- **2309106042** — Muhammad Aidil Saputra *(Active)*
- **2309106044** — Muhammad Rafif Hanif *(Active)*

---

## Judul Proyek Akhir

# Smart Bell

---

## Deskripsi Proyek

Seiring berkembangnya teknologi **Internet of Things (IoT)**, sistem keamanan rumah kini dapat dirancang menjadi lebih cerdas, efisien, dan responsif. Salah satu implementasinya adalah **Smart Bell**, yaitu sistem bel pintar yang mampu mendeteksi keberadaan tamu, memberikan notifikasi secara real-time, serta menampilkan kondisi di depan rumah melalui kamera.

Proyek **Smart Bell** ini dirancang menggunakan **ESP32** sebagai mikrokontroler utama dan **ESP32-CAM** sebagai modul kamera, yang terintegrasi dengan berbagai sensor untuk mendeteksi keberadaan seseorang di depan pintu.

Sistem ini memungkinkan pemilik rumah untuk:

- Mendeteksi keberadaan tamu secara otomatis
- Mengirim notifikasi secara real-time melalui Telegram
- Mengambil gambar kondisi di depan rumah
- Menampilkan status sistem melalui LCD
- Mengaktifkan alarm melalui buzzer dan LED indikator
- Melakukan monitoring jarak jauh menggunakan platform IoT

Dengan adanya sistem ini, pemilik rumah tetap dapat memantau kedatangan tamu meskipun sedang tidak berada di rumah.

---

## Pembagian Tugas

### Muhammad Ihsan
- Implementasi Telegram Bot
- Implementasi MQTT
- Penyusunan laporan

### Muhammad Rafif Hanif
- Perancangan dan perakitan alat
- Penulisan logika program utama
- Pengujian sistem
- Penyusunan laporan

### Muhammad Aidil Saputra
- Integrasi platform IoT menggunakan Thinger.io
- Pembuatan skematik rangkaian
- Penyusunan laporan

---

## Komponen yang Digunakan

- Breadboard
- ESP32 
- ESP32-CAM
- LCD I2C Display
- Buzzer
- LED Indicator 
- Push Button
- Sensor Ultrasonic HC-SR04 
- Sensor PIR

---

## Board Schematic

<p align="center">
  <img src="https://i.ibb.co/RTzpqTMJ/Picture1.png" width="700">
</p>

---

## Platform yang Digunakan

- Arduino IDE
- Thinger.io
- Telegram Bot API
- HiveMQ Cloud MQTT
- Fritzing
