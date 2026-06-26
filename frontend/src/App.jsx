import { useState, useRef, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Link } from 'react-router-dom'
import VectorHeatmap from './VectorHeatmap.jsx'
import './index.css'

function App() {
  const [query, setQuery] = useState('')
  const [kInput, setKInput] = useState("15")
  const [k, setK] = useState(15)
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
      const API = import.meta.env.PROD
        ? 'https://fugginghace26-vectordb.hf.space'
        : 'http://localhost:8000'
      const res = await fetch(`${API}/search?word=${encodeURIComponent(query.trim())}&k=${k}`)
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

      {/* Navbar */}
      <nav className="navbar">
        <div className="nav-left">
          <span className="nav-brand">VectorSearch</span>
        </div>
        <div className="nav-right">
          <Link to="/architecture" className="nav-link">Architecture</Link>
          <a href="https://github.com/" target="_blank" rel="noreferrer" className="nav-icon">
            <svg viewBox="0 0 24 24" fill="currentColor" width="24" height="24">
              <path d="M12 0C5.374 0 0 5.373 0 12c0 5.302 3.438 9.8 8.207 11.387.6.113.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23A11.509 11.509 0 0112 5.803c1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576C20.566 21.797 24 17.3 24 12c0-6.627-5.373-12-12-12z" />
            </svg>
          </a>
        </div>
      </nav>

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
            HNSW · AVX2 · C++17 · FastAPI · React
          </div>
          <h1 className="hero-title">
            Vector Search<br />
            <span className="hero-gradient">Engine</span>
          </h1>
          <p className="hero-sub">
            Approximate nearest neighbor queries powered by a hand-written HNSW graph with explicit SIMD intrinsics. Type any word.
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

            <input
              id="k-input"
              type="number"
              className="search-k-input"
              value={kInput}
              onChange={(e) => setKInput(e.target.value)}
              onBlur={() => {
                const val = Number(kInput);
                console.log(val);

                if (val >= 3 && val <= 50 && Number.isInteger(val)) {
                  setK(val);
                } else {
                  setKInput(String(k));
                }
              }}
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

      {/* Footer */}
      <footer className="footer">
        <p>Built with ❤️ by Tanishq. <br className="mobile-break" /> High-performance and distributed systems enthusiast.</p>
        <div className="footer-links">
          <a href="https://github.com/randomfunction">GitHub</a>
          <a href="https://www.linkedin.com/in/tanishq52/">LinkedIn</a>
        </div>
      </footer>
    </div>
  )
}

export default App
