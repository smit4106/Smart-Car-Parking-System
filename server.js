const express = require("express");
const mongoose = require("mongoose");
const cors = require("cors");

const app = express();
app.use(express.json());
app.use(cors());

// ---------------- MONGODB CONNECTION ----------------
mongoose.connect(
  "mongodb+srv://smitmaniya0401_db_user:um5c4UqNKaJ5LxwN@iotproject.z3toxsp.mongodb.net/smart_parking?retryWrites=true&w=majority"
)
.then(() => console.log("MongoDB Connected"))
.catch(err => console.log(err));


// ---------------- UPDATED SCHEMA (created_at removed) ----------------
const parkingSchema = new mongoose.Schema({

  slot: {
    type: Number,
    required: true
  },

  entry_time: {
    type: Date,
    required: true
  },

  exit_time: {
    type: Date,
    required: true
  },

  duration_seconds: {
    type: Number,
    required: true
  },

  charge_rupees: {
    type: Number,
    required: true
  }

});

const Parking = mongoose.model("parking_logs", parkingSchema);


// ---------------- POST API ----------------
app.post("/log", async (req, res) => {

  try {

    const { slot, entry_time, exit_time, duration_sec, charge } = req.body;

    const entryDate = new Date(entry_time);
    const exitDate = new Date(exit_time);

    const calculatedCharge = charge ? charge : duration_sec;

    const newLog = new Parking({
      slot: slot,
      entry_time: entryDate,
      exit_time: exitDate,
      duration_seconds: duration_sec,
      charge_rupees: calculatedCharge
    });

    await newLog.save();

    res.status(200).json({ message: "Data Saved Successfully" });

  } catch (error) {

    console.log(error);
    res.status(500).json({ message: "Error Saving Data" });

  }
});


// ---------------- DASHBOARD ----------------
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

    totalCollection += log.charge_rupees;

    html += `
    <tr>
      <td>${log.slot}</td>
      <td>${log.entry_time.toLocaleString()}</td>
      <td>${log.exit_time.toLocaleString()}</td>
      <td>${log.duration_seconds}</td>
      <td>${log.charge_rupees}</td>
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
