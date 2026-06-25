import { useRef, useEffect } from 'react'

export default function VectorHeatmap({ vector, height = 32, accent = false }) {
  const canvasRef = useRef(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !vector || vector.length === 0) return

    const ctx = canvas.getContext('2d')
    const w = canvas.width
    const h = canvas.height

    ctx.clearRect(0, 0, w, h)

    const min = Math.min(...vector)
    const max = Math.max(...vector)
    const range = max - min || 1

    const barWidth = w / vector.length

    for (let i = 0; i < vector.length; i++) {
      const norm = (vector[i] - min) / range

      let r, g, b
      if (accent) {
        // Purple-cyan gradient for query vector
        r = Math.floor(100 + norm * 120)
        g = Math.floor(20 + norm * 200)
        b = Math.floor(200 + norm * 55)
      } else {
        // Cool teal-green for results
        r = Math.floor(10 + norm * 40)
        g = Math.floor(180 * norm + 60)
        b = Math.floor(120 + norm * 100)
      }

      ctx.fillStyle = `rgb(${r}, ${g}, ${b})`
      ctx.fillRect(Math.floor(i * barWidth), 0, Math.ceil(barWidth) + 1, h)
    }
  }, [vector, accent])

  return (
    <canvas
      ref={canvasRef}
      width={900}
      height={height}
      className="vector-canvas"
    />
  )
}
