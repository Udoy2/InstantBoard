# 🎨 InstantBoard - Collaborative Drawing Application 🖌️

Welcome to **InstantBoard**, a collaborative drawing application that allows multiple users to draw, erase, and create shapes in real-time! This project is built using **OpenGL** for rendering and **Winsock** for networking, making it a fun and interactive tool for creative collaboration.

---
## 🎥 Video Demonstration

<video width="640" height="360" controls>
  <source src="https://github.com/Udoy2/InstantBoard/blob/main/assets/demonstration.m4v" type="video/mp4">
  Your browser does not support the video tag.
</video>


## 🌟 Features

- **Real-Time Collaboration**:
  - Draw, erase, and create shapes with other users in real-time.
  - Synchronized boards across all connected clients.

- **Drawing Tools**:
  - **Pencil** ✏️: Draw freehand lines.
  - **Eraser** 🧽: Erase parts of the drawing.
  - **Circle** ⚪: Draw perfect circles.
  - **Square** ⬛: Draw squares or rectangles.

- **Multiple Boards**:
  - Create up to **5 boards** and switch between them seamlessly.
  - Each board maintains its own set of strokes and settings.

- **Color Picker** 🎨:
  - Choose any color using the HSV color wheel.
  - Preview the selected color before applying it.

- **Undo Functionality**:
  - Undo the last stroke with a single click or keyboard shortcut.

- **Responsive UI**:
  - Toggle sidebars for tools and board management.
  - Adjustable canvas size and scrollable board thumbnails.

---

## 🛠️ Technologies Used

- **OpenGL**: For rendering 2D graphics.
- **GLUT**: For window management and user input handling.
- **Winsock**: For networking and real-time communication.
- **C++**: The core programming language used for the application.

---

## 🚀 Getting Started

### Prerequisites

- **Windows OS**: This application is designed for Windows.
- **Code Blocks**: Recommended IDE for building and running the project.
- **OpenGL and GLUT**: Ensure these libraries are installed and configured.

### Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Udoy2/InstantBoard.git
   cd InstantBoard
   ```

2. **Open the Project in Code Blocks**:
   - Open the `.cdp` file in Code Blocks.

3. **Build the Project**:
   - Build the solution using the `Build` menu in Code Blocks.

4. **Run the Application**:
   - After building the application run it in terminal

---

## 🖥️ Usage

### Hosting a Session

To host a drawing session, run the application with the `-host` flag:
```bash
InstantBoard.exe -host
```

### Joining a Session

To join a hosted session, run the application with the `-connect` flag followed by the host's IP address:
```bash
InstantBoard.exe -connect 192.168.1.100
```

### Without Session

To run the application without creating any session:
```bash
InstantBoard.exe
```

### Controls

- **Mouse**:
  - Left-click to draw or interact with UI elements.
  - Scroll to navigate through board thumbnails.

- **Keyboard Shortcuts**:
  - `P`: Switch to the **Pencil** tool.
  - `E`: Switch to the **Eraser** tool.
  - `C`: Switch to the **Circle** tool.
  - `S`: Switch to the **Square** tool.
  - `U`: **Undo** the last stroke.
  - `[` and `]`: Decrease or increase the brush size.

---


## 📂 Project Structure

- **`main.cpp`**: The main application logic, including rendering, networking, and user input handling.
- **`Board` Struct**: Manages the state of each drawing board.
- **`Stroke` Struct**: Represents a single stroke (a collection of lines).
- **`Line` Struct**: Represents a single line segment within a stroke.

---

## 🤝 Contributing

Contributions are welcome! If you'd like to contribute to this project, please follow these steps:

1. Fork the repository.
2. Create a new branch for your feature or bugfix.
3. Commit your changes and push to your fork.
4. Submit a pull request with a detailed description of your changes.

---

## 📜 License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **OpenGL Community**: For providing an excellent graphics library.
- **Winsock Documentation**: For making networking in Windows straightforward.
- **Contributors**:
  - [@Udoy2](https://github.com/Udoy2) (Udoy Rahman)
  - [@maniul33](https://github.com/maniul33) (Maniul Islam)
  - [@MariaSultana17](https://github.com/MariaSultana17) (Maria Sultana)
  - [@Pranto2003](https://github.com/Pranto2003) (Pranto Goswamee)
- **You**: For using and contributing to this project! 🎉

---

Your contributions have been invaluable in making this project a success. Thank you for your hard work and dedication! 🙌✨

---

## 📧 Contact

For questions or feedback, feel free to reach out:

- **Email**: udoyrahman983@gmail.com
- **GitHub**: [udoy2](https://github.com/Udoy2)

---

Happy drawing! 🎨✨