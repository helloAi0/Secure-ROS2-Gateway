import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  base: '/Secure-ROS2-Gateway/',
  plugins: [react(), tailwindcss()],
})
