import { useState, useEffect, useRef } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Link } from 'react-router-dom'
import '../index.css'

/* ═══════════════════════════════════════════════════════
   ARCHITECTURE DATA — derived from reading the actual codebase
   ═══════════════════════════════════════════════════════ */

const LAYERS = [
  {
    id: 'frontend',
    label: 'Frontend Layer',
    color: '#22d3ee',
    colorDim: 'rgba(34,211,238,0.12)',
    nodes: [
      {
        id: 'react-ui',
        label: 'React 18 + Vite',
        description: 'Single-page application built with React 18 and Vite dev server. Handles user input, state management, and renders search results with animated transitions via Framer Motion.',
        files: ['frontend/src/App.jsx', 'frontend/src/main.jsx', 'frontend/vite.config.js'],
        badge: null,
      },
      {
        id: 'vector-heatmap',
        label: 'Vector Heatmap',
        description: 'Canvas-based visualization component that renders 300-dimensional FastText embeddings as a colorful barcode. Each of the 300 floats is mapped to a colored pixel strip using min-max normalization.',
        files: ['frontend/src/VectorHeatmap.jsx'],
        badge: null,
      },
      {
        id: 'search-ui',
        label: 'Search Interface',
        description: 'Glassmorphism search bar with configurable K parameter (3–50). Sends GET requests to the FastAPI backend with the query word and K value as URL parameters.',
        files: ['frontend/src/App.jsx'],
        badge: null,
      },
    ],
  },
  {
    id: 'api',
    label: 'API Layer',
    color: '#a78bfa',
    colorDim: 'rgba(167,139,250,0.12)',
    nodes: [
      {
        id: 'fastapi',
        label: 'FastAPI Server',
        description: 'ASGI server running on Uvicorn. Manages application lifecycle via an async context manager that initializes the FastText model and C++ engine at startup. Exposes a single REST endpoint.',
        files: ['app.py'],
        badge: null,
      },
      {
        id: 'search-endpoint',
        label: 'GET /search',
        description: 'Accepts query params: word (string), k (int). Generates query vector via FastText, calls the C++ engine with search_k = k+10, then filters results using Levenshtein distance (threshold < 0.7) to remove near-duplicates.',
        files: ['app.py'],
        badge: null,
      },
      {
        id: 'cors',
        label: 'CORS Middleware',
        description: 'FastAPI CORSMiddleware configured with allow_origins=["*"] to permit cross-origin requests from the Vite dev server (:5173) to the API server (:8000).',
        files: ['app.py'],
        badge: null,
      },
    ],
  },
  {
    id: 'embedding',
    label: 'Embedding Layer',
    color: '#f59e0b',
    colorDim: 'rgba(245,158,11,0.12)',
    nodes: [
      {
        id: 'fasttext',
        label: 'FastText cc.en.300',
        description: 'Facebook\'s pre-trained FastText model (cc.en.300.bin, ~7GB). Generates 300-dimensional word vectors using character n-gram subwords, which natively solves the Out-of-Vocabulary (OOV) problem for misspelled or novel words.',
        files: ['cc.en.300.bin'],
        badge: 'OOV-Tolerant',
      },
      {
        id: 'levenshtein',
        label: 'Levenshtein Filter',
        description: 'Post-search string similarity filter using the python-Levenshtein library. Compares each result\'s metadata string against the query word, discarding results with Levenshtein ratio ≥ 0.7 to suppress trivial morphological variants.',
        files: ['app.py'],
        badge: null,
      },
    ],
  },
  {
    id: 'bindings',
    label: 'Python ↔ C++ Bridge',
    color: '#ec4899',
    colorDim: 'rgba(236,72,153,0.12)',
    nodes: [
      {
        id: 'pybind11',
        label: 'pybind11 Module',
        description: 'Compiled shared library (custom_vectordb.cpython-312-x86_64-linux-gnu.so). Exposes the C++ Engine class, SearchResult struct, and MetricType enum to Python. Uses pybind11/stl.h for automatic std::vector ↔ Python list conversion.',
        files: ['src/bindings.cpp', 'CMakeLists.txt'],
        badge: 'Zero-Copy STL',
      },
    ],
  },
  {
    id: 'engine',
    label: 'Native C++ Engine',
    color: '#34d399',
    colorDim: 'rgba(52,211,153,0.12)',
    nodes: [
      {
        id: 'hnsw-index',
        label: 'HNSW Index',
        description: 'Multi-layer Hierarchical Navigable Small World graph. Parameters: M=32, M_max_0=64, ef_construction=400, ef_search=200, max_capacity=1,000,000. Layer assignment uses -log(uniform_random) * 1/ln(M). Search greedily descends from top layer to layer 0.',
        files: ['include/database.hpp', 'src/database.cpp'],
        badge: 'ANN',
      },
      {
        id: 'distance-metrics',
        label: 'Distance Metrics',
        description: 'Three distance functions: Squared Euclidean (L2), Dot Product, and Cosine Similarity. All three use explicit AVX2 intrinsics: _mm256_loadu_ps for unaligned loads, _mm256_fmadd_ps for fused multiply-add, with a scalar tail loop for remaining dimensions.',
        files: ['src/database.cpp', 'include/database.hpp'],
        badge: 'AVX2 SIMD',
      },
      {
        id: 'memory-arena',
        label: 'Memory Arena',
        description: 'Custom AlignedAllocator<T> template providing 64-byte aligned memory via aligned_alloc(). Used by flat_vectors (vector<float>) and flat_edges (vector<int>) to guarantee cache-line alignment for SIMD loads and maximize L1/L2 cache hit rates.',
        files: ['include/database.hpp'],
        badge: 'Cache-Aligned',
      },
      {
        id: 'graph-ops',
        label: 'Graph Operations',
        description: 'Core HNSW operations: insert() with write lock, search() with shared read lock, search_layer() with thread-local visited arrays and candidate heaps, prune_connections() for edge trimming when neighbor count exceeds max_conn. Uses shared_mutex for concurrent read access.',
        files: ['src/database.cpp'],
        badge: 'Thread-Safe',
      },
    ],
  },
  {
    id: 'storage',
    label: 'Persistence Layer',
    color: '#6366f1',
    colorDim: 'rgba(99,102,241,0.12)',
    nodes: [
      {
        id: 'binary-serialization',
        label: 'Binary Serialization',
        description: 'Custom binary format using raw reinterpret_cast writes. Serializes: metric type, node count, dimension, entry point, max layer, then per-node data (uuid, flat vector, metadata, active flag, HNSW layer, edge offset), followed by the complete flat_edges array.',
        files: ['src/Save_file.cpp'],
        badge: null,
      },
      {
        id: 'snapshot-bin',
        label: 'snapshot.bin',
        description: 'Persistent graph snapshot file (~75MB for 50K words). On startup, if snapshot.bin exists, the engine bypasses the insertion pipeline entirely and deserializes directly from disk. Otherwise, it builds the index from the FastText model and saves a new snapshot.',
        files: ['app.py', 'src/Save_file.cpp'],
        badge: 'Warm Start',
      },
    ],
  },
]

const DATA_FLOW_STEPS = [
  { from: 'search-ui', to: 'search-endpoint', label: 'HTTP GET /search?word=king&k=15' },
  { from: 'search-endpoint', to: 'fasttext', label: 'ft_model.get_word_vector(word)' },
  { from: 'fasttext', to: 'pybind11', label: 'vec: List[float] (300D)' },
  { from: 'pybind11', to: 'hnsw-index', label: 'db.search(vec, k)' },
  { from: 'hnsw-index', to: 'distance-metrics', label: 'compute_distance() per candidate' },
  { from: 'distance-metrics', to: 'hnsw-index', label: 'Top-K SearchResult[]' },
  { from: 'hnsw-index', to: 'pybind11', label: 'vector<SearchResult> → Python list' },
  { from: 'pybind11', to: 'search-endpoint', label: 'Levenshtein filter + JSON' },
  { from: 'search-endpoint', to: 'react-ui', label: 'JSON { query_vector, results[] }' },
]

/* ═══════════════════════════════════════════════════════
   HELPER: find which layer a node belongs to
   ═══════════════════════════════════════════════════════ */
function getNodeLayer(nodeId) {
  for (const layer of LAYERS) {
    for (const node of layer.nodes) {
      if (node.id === nodeId) return layer
    }
  }
  return null
}

/* ═══════════════════════════════════════════════════════
   INSPECTOR PANEL (right side)
   ═══════════════════════════════════════════════════════ */
function InspectorPanel({ node, layer, onClose }) {
  if (!node) return null
  return (
    <motion.div
      className="arch-inspector"
      initial={{ opacity: 0, x: 40 }}
      animate={{ opacity: 1, x: 0 }}
      exit={{ opacity: 0, x: 40 }}
      transition={{ duration: 0.25 }}
    >
      <div className="inspector-header">
        <div className="inspector-dot" style={{ background: layer.color }} />
        <span className="inspector-layer">{layer.label}</span>
        <button className="inspector-close" onClick={onClose}>✕</button>
      </div>
      <h2 className="inspector-title">{node.label}</h2>
      {node.badge && (
        <span className="inspector-badge" style={{ borderColor: layer.color, color: layer.color }}>
          {node.badge}
        </span>
      )}
      <p className="inspector-desc">{node.description}</p>
      <div className="inspector-files">
        <span className="inspector-files-label">Source Files</span>
        {node.files.map((f) => (
          <code key={f} className="inspector-file">{f}</code>
        ))}
      </div>
    </motion.div>
  )
}

/* ═══════════════════════════════════════════════════════
   ARCHITECTURE NODE
   ═══════════════════════════════════════════════════════ */
function ArchNode({ node, color, isActive, onClick, delay }) {
  return (
    <motion.div
      className={`arch-node ${isActive ? 'arch-node--active' : ''}`}
      style={{ '--node-color': color }}
      onClick={() => onClick(node)}
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ delay, duration: 0.5 }}
      whileHover={{ y: -4, transition: { duration: 0.2 } }}
    >
      <div className="arch-node-glow" />
      <div className="arch-node-inner">
        <span className="arch-node-label">{node.label}</span>
        {node.badge && (
          <span className="arch-node-badge" style={{ background: color }}>
            {node.badge}
          </span>
        )}
      </div>
    </motion.div>
  )
}

/* ═══════════════════════════════════════════════════════
   ARCHITECTURE LAYER
   ═══════════════════════════════════════════════════════ */
function ArchLayer({ layer, layerIndex, activeNodeId, onNodeClick }) {
  return (
    <motion.div
      className="arch-layer"
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ delay: layerIndex * 0.12 }}
    >
      <div className="arch-layer-header">
        <div className="arch-layer-indicator" style={{ background: layer.color }} />
        <span className="arch-layer-label">{layer.label}</span>
        <div className="arch-layer-line" style={{ background: `linear-gradient(90deg, ${layer.color}40, transparent)` }} />
      </div>
      <div className="arch-layer-nodes">
        {layer.nodes.map((node, ni) => (
          <ArchNode
            key={node.id}
            node={node}
            color={layer.color}
            isActive={activeNodeId === node.id}
            onClick={onNodeClick}
            delay={layerIndex * 0.12 + ni * 0.06}
          />
        ))}
      </div>
    </motion.div>
  )
}

/* ═══════════════════════════════════════════════════════
   DATA FLOW VISUALIZATION
   ═══════════════════════════════════════════════════════ */
function DataFlow({ steps, activeStep }) {
  return (
    <div className="dataflow">
      <h3 className="dataflow-title">Query Execution Pipeline</h3>
      <div className="dataflow-track">
        {steps.map((step, i) => {
          const fromLayer = getNodeLayer(step.from)
          const toLayer = getNodeLayer(step.to)
          return (
            <motion.div
              className={`dataflow-step ${activeStep === i ? 'dataflow-step--active' : ''}`}
              key={i}
              initial={{ opacity: 0, x: -20 }}
              animate={{ opacity: 1, x: 0 }}
              transition={{ delay: 0.8 + i * 0.07 }}
            >
              <div className="dataflow-step-num">{i + 1}</div>
              <div className="dataflow-step-body">
                <div className="dataflow-step-endpoints">
                  <span className="dataflow-chip" style={{ borderColor: fromLayer?.color }}>
                    {step.from.replace(/-/g, ' ')}
                  </span>
                  <svg className="dataflow-arrow" viewBox="0 0 24 12" fill="none">
                    <path d="M0 6h20m0 0l-4-4m4 4l-4 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
                  </svg>
                  <span className="dataflow-chip" style={{ borderColor: toLayer?.color }}>
                    {step.to.replace(/-/g, ' ')}
                  </span>
                </div>
                <code className="dataflow-step-label">{step.label}</code>
              </div>
            </motion.div>
          )
        })}
      </div>
    </div>
  )
}

/* ═══════════════════════════════════════════════════════
   LEGEND
   ═══════════════════════════════════════════════════════ */
function Legend() {
  return (
    <motion.div
      className="arch-legend"
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ delay: 1.2 }}
    >
      {LAYERS.map((l) => (
        <div className="arch-legend-item" key={l.id}>
          <div className="arch-legend-dot" style={{ background: l.color }} />
          <span>{l.label}</span>
        </div>
      ))}
    </motion.div>
  )
}

/* ═══════════════════════════════════════════════════════
   MAIN PAGE COMPONENT
   ═══════════════════════════════════════════════════════ */
export default function ArchitecturePage() {
  const [activeNode, setActiveNode] = useState(null)
  const [activeStep, setActiveStep] = useState(-1)
  const activeLayer = activeNode ? getNodeLayer(activeNode.id) : null

  // Animate data flow steps sequentially
  useEffect(() => {
    let step = 0
    const interval = setInterval(() => {
      setActiveStep(step)
      step = (step + 1) % DATA_FLOW_STEPS.length
    }, 2000)
    return () => clearInterval(interval)
  }, [])

  return (
    <div className="app">
      <div className="bg-image" />
      <div className="bg-overlay" />
      <div className="bg-grain" />

      <div className="particles">
        {Array.from({ length: 20 }).map((_, i) => (
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
          <Link to="/" className="nav-brand">VectorSearch</Link>
        </div>
        <div className="nav-right">
          <Link to="/" className="nav-link">Search</Link>
          <Link to="/architecture" className="nav-link nav-link--active">Architecture</Link>
          <a href="https://github.com/randomfunction" target="_blank" rel="noreferrer" className="nav-icon">
            <svg viewBox="0 0 24 24" fill="currentColor" width="24" height="24">
              <path d="M12 0C5.374 0 0 5.373 0 12c0 5.302 3.438 9.8 8.207 11.387.6.113.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23A11.509 11.509 0 0112 5.803c1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576C20.566 21.797 24 17.3 24 12c0-6.627-5.373-12-12-12z" />
            </svg>
          </a>
        </div>
      </nav>

      {/* Main content */}
      <div className="arch-page">
        {/* Hero */}
        <motion.div
          className="arch-hero"
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.7 }}
        >
          <h1 className="arch-hero-title">
            System <span className="hero-gradient">Architecture</span>
          </h1>
          <p className="arch-hero-sub">
            End-to-end view of the vector search pipeline — from React UI to AVX2 SIMD distance computation. Every component shown here exists in the repository.
          </p>
        </motion.div>

        <Legend />

        <div className="arch-body">
          {/* Layer stack */}
          <div className="arch-stack">
            {LAYERS.map((layer, li) => (
              <div key={layer.id}>
                <ArchLayer
                  layer={layer}
                  layerIndex={li}
                  activeNodeId={activeNode?.id}
                  onNodeClick={setActiveNode}
                />
                {/* Animated connector between layers */}
                {li < LAYERS.length - 1 && (
                  <div className="arch-connector">
                    <div className="arch-connector-line" />
                    <div className="arch-connector-pulse" />
                  </div>
                )}
              </div>
            ))}
          </div>

          {/* Inspector */}
          <AnimatePresence>
            {activeNode && (
              <InspectorPanel
                node={activeNode}
                layer={activeLayer}
                onClose={() => setActiveNode(null)}
              />
            )}
          </AnimatePresence>
        </div>

        {/* Data flow */}
        <DataFlow steps={DATA_FLOW_STEPS} activeStep={activeStep} />
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
