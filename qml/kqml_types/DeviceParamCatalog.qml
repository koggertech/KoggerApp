pragma Singleton
import QtQuick 2.15

QtObject {
    id: catalog

    readonly property var params: [
        { key: "chart.resolution", group: "echogram", support: "isChartSupport",
          label: qsTr("Resolution, mm"), kind: "spin", prop: "chartResolution",
          from: 10, to: 100, step: 10 },
        { key: "chart.samples", group: "echogram", support: "isChartSupport",
          label: qsTr("Sample count"), kind: "spin", prop: "chartSamples",
          from: 100, to: 15000, step: 100 },
        { key: "chart.offset", group: "echogram", support: "isChartSupport",
          label: qsTr("Offset"), kind: "spin", prop: "chartOffset",
          from: 0, to: 10000, step: 100 },

        { key: "dist.max", group: "rangefinder", support: "isDistSupport",
          label: qsTr("Max distance, mm"), kind: "spin", prop: "distMax",
          from: 0, to: 50000, step: 1000 },
        { key: "dist.deadZone", group: "rangefinder", support: "isDistSupport",
          label: qsTr("Dead zone, mm"), kind: "spin", prop: "distDeadZone",
          from: 0, to: 50000, step: 100 },
        { key: "dist.confidence", group: "rangefinder", support: "isDistSupport",
          label: qsTr("Confidence threshold, %"), kind: "spin", prop: "distConfidence",
          from: 0, to: 100, step: 1 },

        { key: "trans.pulse", group: "transducer", support: "isTransducerSupport",
          label: qsTr("Pulse count"), kind: "spin", prop: "transPulse",
          from: 0, to: 5000, step: 1 },
        { key: "trans.freq", group: "transducer", support: "isTransducerSupport",
          label: qsTr("Frequency, kHz"), kind: "spin", prop: "transFreq",
          from: 40, to: 6000, step: 5 },
        { key: "trans.boost", group: "transducer", support: "isTransducerSupport",
          label: qsTr("Booster"), kind: "switch", prop: "transBoost", onValue: 1 },

        { key: "dsp.horSmooth", group: "dsp", support: "isDSPSupport",
          label: qsTr("Horizontal smoothing"), kind: "spin", prop: "dspHorSmooth",
          from: 0, to: 4, step: 1 },
        { key: "dsp.soundSpeed", group: "dsp", support: "isDSPSupport",
          label: qsTr("Sound speed, m/s"), kind: "spin", prop: "soundSpeed",
          factor: 1000, from: 300, to: 6000, step: 5 },

        { key: "dataset.period", group: "dataset", support: "isDatasetSupport",
          label: qsTr("Period, ms"), kind: "spin", prop: "ch1Period",
          from: 0, to: 2000, step: 50 },
        { key: "dataset.chart", group: "dataset", support: "isDatasetSupport",
          label: qsTr("Echogram"), kind: "tabs", prop: "datasetChart",
          options: [{ label: qsTr("Off"), value: 0 }, { label: qsTr("8-bit"), value: 1 }],
          read: function(dev) { return dev.datasetChart === 1 ? 1 : 0 } },
        { key: "dataset.dist", group: "dataset", support: "isDatasetSupport",
          label: qsTr("Rangefinder"), kind: "tabs",
          props: ["datasetDist", "datasetSDDBT"],
          options: [{ label: qsTr("Off"), value: 0 }, { label: qsTr("On"), value: 1 }, { label: qsTr("NMEA"), value: 2 }],
          read: function(dev) {
              var d = dev.datasetDist, n = dev.datasetSDDBT
              return d === 1 ? 1 : (n === 1 ? 2 : 0)
          },
          write: function(dev, v) {
              if (v === 1)      { dev.datasetDist = 1 }
              else if (v === 2) { dev.datasetSDDBT = 1 }
              else              { dev.datasetDist = 0; dev.datasetSDDBT = 0 }
          } },
        { key: "dataset.ahrs", group: "dataset", support: "isDatasetSupport",
          label: qsTr("AHRS"), kind: "switch", prop: "datasetEuler", mask: 1 },
        { key: "dataset.temperature", group: "dataset", support: "isDatasetSupport",
          label: qsTr("Temperature"), kind: "switch", prop: "datasetTemp", mask: 1 },
        { key: "dataset.timestamp", group: "dataset", support: "isDatasetSupport",
          label: qsTr("Timestamp"), kind: "switch", prop: "datasetTimestamp", mask: 1 }
    ]

    function meta(key) {
        for (var i = 0; i < params.length; ++i)
            if (params[i].key === key)
                return params[i]
        return null
    }

    function has(key) { return meta(key) !== null }

    function supported(dev, key) {
        var m = meta(key)
        if (!dev || !m) return false
        if (!m.support) return true
        return !!dev[m.support]
    }

    function read(dev, key) {
        var m = meta(key)
        if (!dev || !m) return 0
        if (m.read) return m.read(dev)
        var raw = dev[m.prop]
        if (m.kind === "switch")
            return m.mask !== undefined ? ((raw & m.mask) !== 0) : (raw === m.onValue)
        if (m.factor) return Math.round((raw || 0) / m.factor)
        return raw || 0
    }

    function write(dev, key, value) {
        var m = meta(key)
        if (!dev || !m) return
        if (m.write) { m.write(dev, value); return }
        if (m.kind === "switch") {
            dev[m.prop] = value ? (m.mask !== undefined ? m.mask : m.onValue) : 0
            return
        }
        dev[m.prop] = m.factor ? value * m.factor : value
    }
}
