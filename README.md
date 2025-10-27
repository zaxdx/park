
# 🚀 Park

> Sistema monitor para estacionamentos - feito com C++23 e Node-Red

---

## 🧠 Desenvolvido por:

Professor:
- Lucas Feksa

Alunos:
- CHRISTIAN V. DIAS
- DANIELLY C. CARVALHO D.
- RAFAEL D. PEREIRA
- YOUSAF A.

## 🧠 Resumo

**Park** é um sistema para monitorar vagas de estacionamentos de formatos variados,
usando processamento de imagens para detectar as vagas e monitorar a ocupação.
Dadas as delimitações de vagas, o sistema detecta suas posições, se estão ocupadas ou não,
mostrando o layout do estacionamento e situação das vagas via wireless com Node-Red.

---

## ✨ Características de Desenvolvimento

- ⚡ **C++23** — moderno, desenvolvido para performance.
- 🔍 **Node-Red** - front end dinâmico e de rápida customização.
- 🧩 **Desenho Modular** — Back-end e Front-end desacoplados.
- 🧵 **Thread-safe** - usando asio para controle remoto via rede.

---

## 🔧 Compilação

Bibliotecas nescessárias:
```bash
pacman -Syu
pacamn -S v4l-utils v4l2loopback-dkms v4l2loopback-utils 
```

Compilação do back-end:
```bash
git clone https://github.com/zaxdx/park.git
cd park/back-end
make
```
