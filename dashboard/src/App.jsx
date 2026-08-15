import React, { useState, useEffect, useRef } from 'react';
import { Line } from 'react-chartjs-2';
import { 
  Chart as ChartJS, CategoryScale, LinearScale, PointElement, 
  LineElement, Title, Tooltip, Filler, Legend 
} from 'chart.js';
import { 
  ShieldAlert, Cpu, HardDrive, Activity, Server, 
  AlertOctagon, Zap, Info, Terminal, Network, ShieldCheck
} from 'lucide-react';

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Filler, Legend);

export default function App() {
  const [frequency, setFrequency] = useState(1000);
  const [fleetUnit, setFleetUnit] = useState('AMR-01 [Autonomous Rover]');
  const [isThreatActive, setIsThreatActive] = useState(false);
  const [queueDepth, setQueueDepth] = useState(12);
  const [cpuUsage, setCpuUsage] = useState(14);
  const [memUsage, setMemUsage] = useState(256);
  const [chartData, setChartData] = useState(Array(30).fill(1000));
  const [cpuData, setCpuData] = useState(Array(30).fill(14));
  const [logs, setLogs] = useState([]);
  const [payload, setPayload] = useState({});
  const [threatsBlocked, setThreatsBlocked] = useState(0);

  const seqRef = useRef(1000);

  const addLog = (msg, type = 'info') => {
    const time = new Date().toISOString().split('T')[1].slice(0, 12);
    setLogs(prev => [{ time, msg, type }, ...prev].slice(0, 50));
  };

  useEffect(() => {
    addLog('SYSTEM BOOT: ROS 2 FastDDS Discovery initialized on domain 0', 'info');
    addLog('STORAGE: SQLite WAL mode active at /mnt/data/telemetry.db', 'success');
    addLog('CRYPTO: OpenSSL AES-256-GCM hardware acceleration verified', 'success');
  }, []);

  // Complex System Simulation Loop
  useEffect(() => {
    const interval = setInterval(() => {
      seqRef.current += 1;
      const target = Number(frequency);
      
      if (isThreatActive) {
        // Simulating system overload and dropped packets during an attack
        setChartData(prev => [...prev.slice(1), Math.max(0, target * 0.1)]);
        setCpuData(prev => [...prev.slice(1), 98]);
        setCpuUsage(98.5);
        setQueueDepth(100);
      } else {
        // Normal operations
        const noise = (Math.random() - 0.5) * (target * 0.05);
        setChartData(prev => [...prev.slice(1), Math.floor(target + noise)]);
        
        const baseCpu = 10 + (target / 5000) * 40; // CPU scales with frequency
        const currentCpu = baseCpu + (Math.random() * 5);
        setCpuData(prev => [...prev.slice(1), currentCpu]);
        setCpuUsage(currentCpu);
        
        setMemUsage(prev => Math.min(1024, prev + (target / 10000)));
        setQueueDepth(prev => Math.min(100, Math.max(2, prev + (Math.random() * 8 - 4))));

        setPayload({
          "TelemetryPayload": {
            "robot_id": fleetUnit.split(' ')[0],
            "sequence_id": seqRef.current,
            "timestamp_ns": Date.now() * 1000000,
            "qos_profile": "SENSOR_DATA",
            "sensor_data": {
              "linear_velocity_m_s": (Math.random() * 2.5).toFixed(3),
              "angular_velocity_rad_s": (Math.random() * 0.5).toFixed(3),
              "battery_soc_pct": (94.2 - (seqRef.current * 0.001)).toFixed(2)
            },
            "security_tag_verified": true
          }
        });
      }
    }, 100); 

    return () => clearInterval(interval);
  }, [frequency, isThreatActive, fleetUnit]);

  const triggerThreat = () => {
    setIsThreatActive(true);
    addLog('CRITICAL: Invalid HMAC signature detected! Payload tampered in transit.', 'danger');
    addLog('FIREWALL: Dropping malicious packets from unverified publisher.', 'danger');
    setThreatsBlocked(prev => prev + 1);
    
    setTimeout(() => {
      addLog('THREAT RESOLVED: Malicious node isolated. Restoring verified DDS traffic.', 'success');
      setIsThreatActive(false);
      setQueueDepth(15);
    }, 3500);
  };

  const flushWAL = () => {
    addLog('STORAGE: Executed PRAGMA wal_checkpoint(TRUNCATE)', 'info');
    addLog('STORAGE: 45,210 batched records committed to persistent disk.', 'success');
    setQueueDepth(2);
    setMemUsage(256);
  };

  const chartOptions = {
    responsive: true, maintainAspectRatio: false, animation: { duration: 0 },
    scales: {
      x: { display: false },
      y: { grid: { color: 'rgba(255,255,255,0.05)' }, min: 0 },
      y1: { position: 'right', grid: { display: false }, min: 0, max: 100 }
    },
    plugins: { legend: { display: false }, tooltip: { mode: 'index', intersect: false } }
  };

  return (
    <div className="min-h-screen bg-[#09090b] text-zinc-300 p-4 font-sans selection:bg-cyan-900">
      {/* Top Navigation / Header */}
      <header className="flex flex-col lg:flex-row justify-between items-start lg:items-center mb-6 gap-4 border-b border-zinc-800 pb-4">
        <div>
          <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-3">
            <ShieldCheck className="text-cyan-500" />
            SECURE ROS 2 TELEMETRY GATEWAY
          </h1>
          <p className="text-zinc-500 text-sm mt-1">Production Operations Center • Edge C++20 Node</p>
        </div>
        <div className="flex gap-2 text-[10px] font-mono uppercase font-bold">
          <span className="px-3 py-1.5 bg-emerald-950 text-emerald-400 border border-emerald-900/50 rounded flex items-center gap-2">
            <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse"></span> DDS Link Active
          </span>
          <span className="px-3 py-1.5 bg-cyan-950 text-cyan-400 border border-cyan-900/50 rounded flex items-center gap-2">
            <span className="w-1.5 h-1.5 rounded-full bg-cyan-500 animate-pulse"></span> AES-256-GCM
          </span>
          <span className="px-3 py-1.5 bg-zinc-900 text-zinc-400 border border-zinc-800 rounded">SQLite WAL</span>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
        
        {/* Left Column: Instructions & Controls */}
        <div className="lg:col-span-1 flex flex-col gap-6">
          
          {/* Operational Briefing (Instructions) */}
          <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-5">
            <h2 className="text-sm font-bold text-white mb-3 flex items-center gap-2 uppercase tracking-wider">
              <Info size={16} className="text-cyan-500" /> Operational Briefing
            </h2>
            <div className="text-xs text-zinc-400 space-y-3 leading-relaxed">
              <p>
                <strong className="text-zinc-200">What is this?</strong> This is an interactive simulation of a C++ edge security node. It sits between autonomous robots (ROS 2) and the cloud database, encrypting and verifying massive streams of sensor data in real-time.
              </p>
              <p>
                <strong className="text-zinc-200">How to use this dashboard:</strong>
              </p>
              <ul className="list-disc pl-4 space-y-2">
                <li><strong className="text-cyan-400">Increase Ingestion Frequency:</strong> Use the slider below to simulate higher network traffic. Watch how the CPU load and Queue Depth scale in response.</li>
                <li><strong className="text-red-400">Inject Cyber Threat:</strong> Simulates a Man-in-the-Middle (MitM) attack altering the data. Watch the system block the packet, spike CPU for reallocation, and halt throughput to protect the database.</li>
                <li><strong className="text-purple-400">Trigger WAL Flush:</strong> Forces the SQLite database to write its memory buffer to disk. Watch the Ring Buffer and Memory utilization instantly reset.</li>
              </ul>
            </div>
          </div>

          {/* Controls */}
          <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-5">
            <h2 className="text-sm font-bold text-white mb-4 uppercase tracking-wider flex items-center gap-2">
              <Zap size={16} className="text-amber-500" /> System Controls
            </h2>
            
            <div className="mb-5">
              <label className="text-xs text-zinc-500 block mb-2 uppercase font-semibold">Target Fleet Node</label>
              <select 
                value={fleetUnit} onChange={(e) => setFleetUnit(e.target.value)}
                className="w-full bg-black border border-zinc-700 text-xs rounded p-2 text-zinc-300 outline-none focus:border-cyan-500"
              >
                <option>AMR-01 [Autonomous Rover]</option>
                <option>AGV-02 [Warehouse Forklift]</option>
                <option>UAV-03 [Inspection Drone]</option>
              </select>
            </div>

            <div className="mb-6">
              <div className="flex justify-between text-xs font-semibold mb-2">
                <span className="text-zinc-500 uppercase">Traffic Load</span>
                <span className="text-cyan-400 font-mono">{frequency} Hz</span>
              </div>
              <input 
                type="range" min="100" max="5000" step="100" 
                value={frequency} onChange={(e) => setFrequency(e.target.value)}
                className="w-full accent-cyan-500"
              />
            </div>

            <div className="flex flex-col gap-3">
              <button onClick={triggerThreat} className="w-full bg-red-950/50 hover:bg-red-900/80 text-red-500 border border-red-900 p-2.5 rounded text-xs font-bold flex items-center justify-center gap-2 transition-colors uppercase">
                <AlertOctagon size={14} /> Inject Cyber Threat
              </button>
              <button onClick={flushWAL} className="w-full bg-purple-950/50 hover:bg-purple-900/80 text-purple-400 border border-purple-900 p-2.5 rounded text-xs font-bold flex items-center justify-center gap-2 transition-colors uppercase">
                <HardDrive size={14} /> Force WAL Flush
              </button>
            </div>
          </div>
        </div>

        {/* Right Column: Dashboards & Logs */}
        <div className="lg:col-span-3 flex flex-col gap-6">
          
          {/* Hardware & Network Metrics */}
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-4">
              <div className="flex items-center gap-2 text-zinc-500 mb-2"><Cpu size={14}/> <span className="text-[10px] uppercase font-bold">Node CPU</span></div>
              <div className={`text-2xl font-mono font-bold ${cpuUsage > 80 ? 'text-red-500' : 'text-zinc-100'}`}>{cpuUsage.toFixed(1)}%</div>
            </div>
            <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-4">
              <div className="flex items-center gap-2 text-zinc-500 mb-2"><Server size={14}/> <span className="text-[10px] uppercase font-bold">Memory (RSS)</span></div>
              <div className="text-2xl font-mono font-bold text-zinc-100">{memUsage.toFixed(0)} <span className="text-sm text-zinc-500">MB</span></div>
            </div>
            <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-4">
              <div className="flex justify-between items-center mb-2">
                <div className="flex items-center gap-2 text-zinc-500"><Activity size={14}/> <span className="text-[10px] uppercase font-bold">Ring Buffer</span></div>
                <span className="text-[10px] font-mono text-zinc-400">{queueDepth.toFixed(0)}%</span>
              </div>
              <div className="w-full bg-black h-1.5 rounded-full overflow-hidden mt-4">
                <div className={`h-full transition-all duration-200 ${queueDepth > 80 ? 'bg-red-500' : 'bg-amber-500'}`} style={{ width: `${queueDepth}%` }}></div>
              </div>
            </div>
            <div className={`bg-zinc-900/50 border rounded-lg p-4 ${threatsBlocked > 0 ? 'border-red-900/50' : 'border-zinc-800'}`}>
              <div className="flex items-center gap-2 text-zinc-500 mb-2"><ShieldAlert size={14}/> <span className="text-[10px] uppercase font-bold">Threats Blocked</span></div>
              <div className="text-2xl font-mono font-bold text-red-400">{threatsBlocked}</div>
            </div>
          </div>

          {/* Main Telemetry Chart */}
          <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-5">
            <h3 className="text-xs font-bold text-zinc-400 mb-4 uppercase tracking-wider flex items-center gap-2">
              <Network size={14} className="text-cyan-500"/> Network Throughput vs CPU Load
            </h3>
            <div className="h-64">
              <Line 
                data={{
                  labels: Array(30).fill(''),
                  datasets: [
                    {
                      label: 'Throughput (ops/sec)',
                      data: chartData,
                      borderColor: isThreatActive ? '#ef4444' : '#06b6d4',
                      backgroundColor: isThreatActive ? 'rgba(239, 68, 68, 0.1)' : 'rgba(6, 182, 212, 0.1)',
                      fill: true, tension: 0.3, pointRadius: 0, yAxisID: 'y'
                    },
                    {
                      label: 'CPU Load (%)',
                      data: cpuData,
                      borderColor: '#f59e0b',
                      borderDash: [5, 5],
                      borderWidth: 1,
                      fill: false, tension: 0.3, pointRadius: 0, yAxisID: 'y1'
                    }
                  ]
                }}
                options={chartOptions}
              />
            </div>
          </div>

          {/* Bottom Split: Logs & Data */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-5">
              <h3 className="text-xs font-bold text-zinc-400 mb-3 uppercase tracking-wider flex items-center gap-2">
                <Terminal size={14} className="text-emerald-500"/> Live Decrypted Payload
              </h3>
              <pre className="bg-black p-4 rounded text-[10px] font-mono text-emerald-400/90 h-56 overflow-y-auto border border-zinc-800/50 leading-relaxed">
                {JSON.stringify(payload, null, 2)}
              </pre>
            </div>

            <div className="bg-zinc-900/50 border border-zinc-800 rounded-lg p-5">
              <h3 className="text-xs font-bold text-zinc-400 mb-3 uppercase tracking-wider flex items-center gap-2">
                <ShieldAlert size={14} className="text-amber-500"/> System Security Audit
              </h3>
              <div className="bg-black p-4 rounded text-[10px] font-mono h-56 overflow-y-auto border border-zinc-800/50 flex flex-col gap-2.5">
                {logs.map((log, i) => (
                  <div key={i} className={`flex gap-3 border-b border-zinc-900 pb-2 ${log.type === 'danger' ? 'text-red-400' : log.type === 'success' ? 'text-emerald-400' : 'text-zinc-400'}`}>
                    <span className="text-zinc-600 shrink-0">[{log.time}]</span> 
                    <span>{log.msg}</span>
                  </div>
                ))}
              </div>
            </div>
          </div>

        </div>
      </div>
    </div>
  );
}