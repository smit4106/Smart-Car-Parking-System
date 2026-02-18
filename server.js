const express = require("express");
const mongoose = require("mongoose");
const cors = require("cors");

const app = express();
app.use(express.json());
app.use(cors());

// 🔐 MongoDB Connection
mongoose.connect(
  "mongodb+srv://smitmaniya0401_db_user:um5c4UqNKaJ5LxwN@iotproject.z3toxsp.mongodb.net/smart_parking?retryWrites=true&w=majority"
)
.then(() => console.log("MongoDB Connected"))
.catch(err => console.log(err));

// Schema
const parkingSchema = new mongoose.Schema({
  slot: Number,
  entry_time: String,
  exit_time: String,
  duration_seconds: Number,
  charge_rupees: Number   // (optional if ESP32 sends it)
});

const Parking = mongoose.model("parking_logs", parkingSchema);

// ---------------- EXISTING POST API ----------------
app.post("/log", async (req, res) => {
  try {
    const newLog = new Parking(req.body);
    await newLog.save();
    res.status(200).json({ message: "Saved" });
  } catch (error) {
    res.status(500).json({ message: "Error" });
  }
});

// ---------------- NEW TABLE VIEW ROUTE ----------------
app.get("/dashboard", async (req, res) => {

  const logs = await Parking.find().sort({ _id: -1 });

  let html = `
  <h2>Smart Parking Records</h2>
  <table border="1" cellpadding="10" cellspacing="0">
  <tr>
    <th>Slot</th>
    <th>Entry Time</th>
    <th>Exit Time</th>
    <th>Duration (sec)</th>
    <th>Charge (₹)</th>
  </tr>`;

  let totalCollection = 0;

  logs.forEach(log => {

    let charge = log.charge_rupees || log.duration_seconds; // ₹1 per sec if not stored
    totalCollection += charge;

    html += `
    <tr>
      <td>${log.slot}</td>
      <td>${log.entry_time}</td>
      <td>${log.exit_time}</td>
      <td>${log.duration_seconds}</td>
      <td>${charge}</td>
    </tr>`;
  });

  html += `
  </table>
  <h3>Total Collection: ₹${totalCollection}</h3>
  `;

  res.send(html);
});

// ---------------- ROOT ROUTE ----------------
app.get("/", (req, res) => {
  res.send("Smart Parking Server Running");
});

app.listen(3000, () => {
  console.log("Server running on port 3000");
});
