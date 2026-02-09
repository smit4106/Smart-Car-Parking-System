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
  duration_seconds: Number
});

const Parking = mongoose.model("parking_logs", parkingSchema);

// API
app.post("/log", async (req, res) => {
  try {
    const newLog = new Parking(req.body);
    await newLog.save();
    res.status(200).json({ message: "Saved" });
  } catch (error) {
    res.status(500).json({ message: "Error" });
  }
});

app.get("/", (req, res) => {
  res.send("Smart Parking Server Running");
});

app.listen(3000, () => {
  console.log("Server running on port 3000");
});
