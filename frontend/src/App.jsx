import { useState, useRef, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import VectorHeatmap from './VectorHeatmap.jsx'
import './index.css'

function App() {
  const [query, setQuery] = useState('')
  const [loading, setLoading] = useState(false)
  const [data, setData] = useState(null)
  const [searchTime, setSearchTime] = useState(null)
  const inputRef = useRef(null)

  useEffect(() => {
    inputRef.current?.focus()
  }, [])

  const handleSearch = async (e) => {
    e.preventDefault()
    if (!query.trim()) return
    setLoading(true)
    setData(null)
    const t0 = performance.now()
    try {
      const res = await fetch(`http://localhost:8000/search?word=${encodeURIComponent(query.trim())}&k=11`)
      const json = await res.json()
      setSearchTime((performance.now() - t0).toFixed(1))
      setData(json)
    } catch {
      alert('Backend unreachable. Ensure FastAPI is running on :8000')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="app">
      {/* Background layers */}
      <div className="bg-image" />
      <div className="bg-overlay" />
      <div className="bg-grain" />

      {/* Floating particles */}
      <div className="particles">
        {Array.from({ length: 30 }).map((_, i) => (
          <div
            key={i}
            className="particle"
            style={{
              left: `${Math.random() * 100}%`,
              top: `${Math.random() * 100}%`,
              animationDelay: `${Math.random() * 8}s`,
              animationDuration: `${6 + Math.random() * 8}s`,
              width: `${2 + Math.random() * 3}px`,
              height: `${2 + Math.random() * 3}px`,
            }}
          />
        ))}
      </div>

      <div className="content">
        {/* Header */}
        <motion.header
          className="hero"
          initial={{ opacity: 0, y: -30 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8, ease: 'easeOut' }}
        >
          <div className="hero-badge">
            <span className="badge-dot" />
            HNSW · AVX2 · C++17
          </div>
          <h1 className="hero-title">
            Vector Search<br />
            <span className="hero-gradient">Engine</span>
          </h1>
          <p className="hero-sub">
            Approximate nearest neighbor queries powered by a hand-written HNSW graph with explicit SIMD intrinsics. Type any word — even misspelled ones.
          </p>
        </motion.header>

        {/* Search */}
        <motion.form
          className="search-form"
          onSubmit={handleSearch}
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8, delay: 0.2 }}
        >
          <div className="search-wrapper">
            <svg className="search-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="11" cy="11" r="8" />
              <line x1="21" y1="21" x2="16.65" y2="16.65" />
            </svg>
            <input
              ref={inputRef}
              className="search-input"
              type="text"
              placeholder="king, quantum, Einsteinn..."
              value={query}
              onChange={(e) => setQuery(e.target.value)}
            />
            <button className="search-btn" type="submit" disabled={loading}>
              {loading ? (
                <div className="spinner" />
              ) : (
                <>Search</>
              )}
            </button>
          </div>
        </motion.form>

        {/* Loading state */}
        <AnimatePresence>
          {loading && (
            <motion.div
              className="loading-bar-wrapper"
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
            >
              <div className="loading-bar" />
              <p className="loading-text">Traversing HNSW layers...</p>
            </motion.div>
          )}
        </AnimatePresence>

        {/* Results */}
        <AnimatePresence>
          {data && !loading && (
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ duration: 0.5 }}
            >
              {/* Stats bar */}
              <div className="stats-bar">
                <div className="stat">
                  <span className="stat-value">{data.results.length}</span>
                  <span className="stat-label">Results</span>
                </div>
                <div className="stat">
                  <span className="stat-value">{searchTime}ms</span>
                  <span className="stat-label">Roundtrip</span>
                </div>
                <div className="stat">
                  <span className="stat-value">300D</span>
                  <span className="stat-label">Embedding</span>
                </div>
                <div className="stat">
                  <span className="stat-value">COSINE</span>
                  <span className="stat-label">Metric</span>
                </div>
              </div>

              {/* Query vector card */}
              <motion.div
                className="query-card"
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: 0.1 }}
              >
                <div className="query-card-header">
                  <div>
                    <span className="query-label">QUERY VECTOR</span>
                    <h2 className="query-word">"{query}"</h2>
                  </div>
                  <div className="query-dim-badge">300 dimensions</div>
                </div>
                <VectorHeatmap vector={data.query_vector} height={40} accent />
              </motion.div>

              {/* Result cards */}
              <div className="results-list">
                {data.results.map((res, idx) => (
                  <motion.div
                    className="result-card"
                    key={idx}
                    initial={{ opacity: 0, x: -20 }}
                    animate={{ opacity: 1, x: 0 }}
                    transition={{ delay: 0.05 * idx }}
                  >
                    <div className="result-rank">#{idx + 1}</div>
                    <div className="result-body">
                      <div className="result-top">
                        <h3 className="result-word">{res.word}</h3>
                        <div className="result-dist">
                          <span className="dist-value">{res.distance.toFixed(6)}</span>
                          <span className="dist-label">cosine dist</span>
                        </div>
                      </div>
                      <VectorHeatmap vector={res.vector} height={28} />
                    </div>
                  </motion.div>
                ))}
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </div>
    </div>
  )
}

export default App
