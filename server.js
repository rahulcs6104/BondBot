// BondBot Backend Server
// Connects MQTT broker (DigitalOcean) with MongoDB Atlas
// Now includes HTTP endpoint for audio mood analysis via Gemini API

const mqtt = require('mqtt');
const { MongoClient } = require('mongodb');
const http = require('http');

// ========================================
// CONFIGURATION
// ========================================

const config = {
  // MQTT Configuration (Your DigitalOcean Droplet)
  mqtt: {
    host: '159.89.46.71',
    port: 1883,
    username: 'couplebot',
    password: 'CoupleBot2026!',
    clientId: 'bondbot-backend-' + Math.random().toString(16).substr(2, 8)
  },
  
  // MongoDB Configuration (MongoDB Atlas - FREE tier)
  mongodb: {
    uri: 'mongodb+srv://rahulcs6104_db_user:yoiPMuVnSqjpX4iy@cluster0.vsrjygb.mongodb.net/?appName=Cluster0',
    database: 'couple_companion',
    collection: 'pair_state'
  },

  // Gemini API
  gemini: {
    apiKey: 'AIzaSyCtXYv8p8okniSsdKFgiKMdvwCN53wNcb8',
    model: 'gemini-2.0-flash'
  },

  // HTTP Server port for receiving audio from ESP32
  httpPort: 3000
};

// ========================================
// GLOBAL VARIABLES
// ========================================

let mqttClient;
let mongoClient;
let db;
let collection;

// ========================================
// MONGODB CONNECTION
// ========================================

async function connectMongoDB() {
  try {
    console.log('📊 Connecting to MongoDB Atlas...');
    
    mongoClient = new MongoClient(config.mongodb.uri, {
      useNewUrlParser: true,
      useUnifiedTopology: true,
    });
    
    await mongoClient.connect();
    
    db = mongoClient.db(config.mongodb.database);
    collection = db.collection(config.mongodb.collection);
    
    console.log('✅ MongoDB connected!');
    console.log(`   Database: ${config.mongodb.database}`);
    console.log(`   Collection: ${config.mongodb.collection}`);
    
    // Initialize pair_state if it doesn't exist
    await initializePairState();
    
  } catch (error) {
    console.error('❌ MongoDB connection failed:', error.message);
    process.exit(1);
  }
}

async function initializePairState() {
  try {
    const existingPair = await collection.findOne({ pair_id: 'pair01' });
    
    if (!existingPair) {
      console.log('📝 Initializing pair_state document...');
      
      await collection.insertOne({
        pair_id: 'pair01',
        device_a: {
          last_seen: new Date(),
          online: false,
          activity_days: [false, false, false, false, false, false, false]
        },
        device_b: {
          last_seen: new Date(),
          online: false,
          activity_days: [false, false, false, false, false, false, false]
        },
        interactions: [],
        created_at: new Date(),
        updated_at: new Date()
      });
      
      console.log('✅ pair_state initialized for pair01');
    } else {
      console.log('✅ pair_state already exists for pair01');
    }
  } catch (error) {
    console.error('❌ Failed to initialize pair_state:', error.message);
  }
}

// ========================================
// MQTT CONNECTION
// ========================================

function connectMQTT() {
  console.log('🔗 Connecting to MQTT broker...');
  console.log(`   Host: ${config.mqtt.host}:${config.mqtt.port}`);
  
  mqttClient = mqtt.connect(`mqtt://${config.mqtt.host}:${config.mqtt.port}`, {
    clientId: config.mqtt.clientId,
    username: config.mqtt.username,
    password: config.mqtt.password,
    clean: true,
    reconnectPeriod: 5000,
  });
  
  mqttClient.on('connect', () => {
    console.log('✅ MQTT connected!');
    
    // Subscribe to all pair topics
    mqttClient.subscribe('bondbot/+/+', (err) => {
      if (err) {
        console.error('❌ Subscribe failed:', err);
      } else {
        console.log('📥 Subscribed to: bondbot/+/+');
      }
    });
  });
  
  mqttClient.on('message', async (topic, message) => {
    try {
      const payload = JSON.parse(message.toString());
      console.log(`\n📨 Message received:`);
      console.log(`   Topic: ${topic}`);
      console.log(`   Payload:`, payload);
      
      await handleMessage(topic, payload);
      
    } catch (error) {
      console.error('❌ Error processing message:', error.message);
    }
  });
  
  mqttClient.on('error', (error) => {
    console.error('❌ MQTT Error:', error.message);
  });
  
  mqttClient.on('offline', () => {
    console.log('⚠️  MQTT offline, attempting to reconnect...');
  });
  
  mqttClient.on('reconnect', () => {
    console.log('🔄 Reconnecting to MQTT...');
  });
}

// ========================================
// MESSAGE HANDLERS
// ========================================

async function handleMessage(topic, payload) {
  const { type, from, pair_id } = payload;
  
  if (!pair_id) {
    console.log('⚠️  Message missing pair_id, ignoring');
    return;
  }
  
  // Update device presence
  await updateDevicePresence(pair_id, from);
  
  // Store interaction
  const interaction = {
    type: type,
    from_device: from,
    timestamp: new Date(),
    data: payload
  };
  
  await storeInteraction(pair_id, interaction);
  
  // Handle specific message types
  switch (type) {
    case 'PRESENCE_PING':
      console.log(`   → ${from} sent presence ping`);
      break;
      
    case 'CHECKIN_REQUEST':
      console.log(`   → ${from} wants to check in`);
      break;
      
    case 'AUDIO_MESSAGE':
      console.log(`   → ${from} sent gratitude message`);
      break;
      
    case 'ACTIVITY_UPDATE':
      console.log(`   → ${from} updated activity (day ${payload.day})`);
      await updateActivity(pair_id, from, payload.day);
      break;

    case 'MOOD_RESULT':
      console.log(`   → Mood result: ${payload.mood} for ${payload.target}`);
      break;
      
    default:
      console.log(`   → Unknown message type: ${type}`);
  }
}

// ========================================
// DATABASE OPERATIONS
// ========================================

async function updateDevicePresence(pairId, deviceId) {
  try {
    const deviceField = deviceId === 'A' ? 'device_a' : 'device_b';
    
    await collection.updateOne(
      { pair_id: pairId },
      {
        $set: {
          [`${deviceField}.last_seen`]: new Date(),
          [`${deviceField}.online`]: true,
          updated_at: new Date()
        }
      }
    );
    
    console.log(`   ✔ Updated ${deviceId} presence`);
  } catch (error) {
    console.error('❌ Failed to update presence:', error.message);
  }
}

async function storeInteraction(pairId, interaction) {
  try {
    await collection.updateOne(
      { pair_id: pairId },
      {
        $push: { interactions: interaction },
        $set: { updated_at: new Date() }
      }
    );
    
    console.log(`   ✔ Stored interaction: ${interaction.type}`);
  } catch (error) {
    console.error('❌ Failed to store interaction:', error.message);
  }
}

async function updateActivity(pairId, deviceId, day) {
  try {
    const deviceField = deviceId === 'A' ? 'device_a' : 'device_b';
    
    await collection.updateOne(
      { pair_id: pairId },
      {
        $set: {
          [`${deviceField}.activity_days.${day}`]: true,
          updated_at: new Date()
        }
      }
    );
    
    console.log(`   ✔ Updated ${deviceId} activity for day ${day}`);
  } catch (error) {
    console.error('❌ Failed to update activity:', error.message);
  }
}

// ========================================
// GEMINI MOOD ANALYSIS
// ========================================

async function analyzeAudioMood(audioBase64, fromDevice, pairId, requesterDevice) {
  try {
    console.log(`\n🧠 Analyzing mood for ${fromDevice}'s audio...`);
    console.log(`   Audio size: ${audioBase64.length} chars base64`);
    
    // Call Gemini API with audio
    const requestBody = JSON.stringify({
      contents: [{
        parts: [
          {
            inline_data: {
              mime_type: "audio/wav",
              data: audioBase64
            }
          },
          {
            text: "Listen to this short audio clip of a person speaking. Analyze their voice tone, pitch, energy, and emotional state. Based on your analysis, classify their mood as exactly one of these three words: happy, neutral, or sad. Respond with ONLY one word: happy, neutral, or sad. Nothing else."
          }
        ]
      }],
      generationConfig: {
        temperature: 0.1,
        maxOutputTokens: 10
      }
    });

    const url = `https://generativelanguage.googleapis.com/v1beta/models/${config.gemini.model}:generateContent?key=${config.gemini.apiKey}`;
    
    const response = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: requestBody
    });

    const data = await response.json();
    
    console.log('   Gemini raw response:', JSON.stringify(data).substring(0, 200));
    
    let mood = 'neutral'; // default
    
    if (data.candidates && data.candidates[0] && data.candidates[0].content) {
      const text = data.candidates[0].content.parts[0].text.toLowerCase().trim();
      console.log(`   Gemini says: "${text}"`);
      
      if (text.includes('happy')) mood = 'happy';
      else if (text.includes('sad')) mood = 'sad';
      else mood = 'neutral';
    } else {
      console.log('   ⚠️ Unexpected Gemini response, defaulting to neutral');
      if (data.error) {
        console.log('   Gemini error:', data.error.message);
      }
    }
    
    console.log(`   🎭 Detected mood: ${mood}`);
    
    // Publish mood result back via MQTT to the requester
    // If A requested and B recorded, send result to A (on B_to_A topic)
    const resultTopic = requesterDevice === 'A' 
      ? `bondbot/${pairId}/B_to_A` 
      : `bondbot/${pairId}/A_to_B`;
    
    const moodMessage = JSON.stringify({
      type: 'MOOD_RESULT',
      mood: mood,
      from: fromDevice,
      target: requesterDevice,
      pair_id: pairId
    });
    
    mqttClient.publish(resultTopic, moodMessage);
    console.log(`   📤 Sent mood result "${mood}" to ${requesterDevice} on ${resultTopic}`);
    
    // Store in MongoDB
    await storeInteraction(pairId, {
      type: 'MOOD_ANALYSIS',
      from_device: fromDevice,
      target_device: requesterDevice,
      mood: mood,
      timestamp: new Date()
    });
    
    return mood;
    
  } catch (error) {
    console.error('❌ Gemini analysis failed:', error.message);
    
    // Send neutral as fallback
    const resultTopic = requesterDevice === 'A' 
      ? `bondbot/${pairId}/B_to_A` 
      : `bondbot/${pairId}/A_to_B`;
    
    mqttClient.publish(resultTopic, JSON.stringify({
      type: 'MOOD_RESULT',
      mood: 'neutral',
      from: fromDevice,
      target: requesterDevice,
      pair_id: pairId
    }));
    
    return 'neutral';
  }
}

// ========================================
// HTTP SERVER (receives audio from ESP32)
// ========================================

function startHTTPServer() {
  const server = http.createServer((req, res) => {
    // CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    
    if (req.method === 'OPTIONS') {
      res.writeHead(200);
      res.end();
      return;
    }

    // Health check endpoint
    if (req.method === 'GET' && req.url === '/health') {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ status: 'ok', service: 'bondbot-backend' }));
      return;
    }
    
    // Audio upload endpoint
    if (req.method === 'POST' && req.url === '/analyze-mood') {
      let body = '';
      
      req.on('data', (chunk) => {
        body += chunk.toString();
        
        // Limit to 1MB
        if (body.length > 1024 * 1024) {
          res.writeHead(413, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: 'Audio too large' }));
          req.destroy();
        }
      });
      
      req.on('end', async () => {
        try {
          console.log(`\n📥 Received audio upload (${body.length} bytes)`);
          
          const data = JSON.parse(body);
          const { audio, from, pair_id, requester } = data;
          
          if (!audio || !from || !pair_id || !requester) {
            res.writeHead(400, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ error: 'Missing fields: audio, from, pair_id, requester' }));
            return;
          }
          
          console.log(`   From: Device ${from}, Requester: Device ${requester}, Pair: ${pair_id}`);
          
          // Analyze mood via Gemini
          const mood = await analyzeAudioMood(audio, from, pair_id, requester);
          
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ mood: mood, status: 'ok' }));
          
        } catch (error) {
          console.error('❌ Error processing audio:', error.message);
          res.writeHead(500, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: error.message }));
        }
      });
      
      return;
    }
    
    // 404 for everything else
    res.writeHead(404, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'Not found' }));
  });
  
  server.listen(config.httpPort, '0.0.0.0', () => {
    console.log(`🌐 HTTP server listening on port ${config.httpPort}`);
    console.log(`   Audio endpoint: http://159.89.46.71:${config.httpPort}/analyze-mood`);
  });
}

// ========================================
// PERIODIC TASKS
// ========================================

// Mark devices as offline if no activity for 5 minutes
setInterval(async () => {
  try {
    const fiveMinutesAgo = new Date(Date.now() - 5 * 60 * 1000);
    
    await collection.updateMany(
      {
        $or: [
          { 'device_a.last_seen': { $lt: fiveMinutesAgo } },
          { 'device_b.last_seen': { $lt: fiveMinutesAgo } }
        ]
      },
      {
        $set: {
          'device_a.online': false,
          'device_b.online': false
        }
      }
    );
  } catch (error) {
    console.error('❌ Error updating offline status:', error.message);
  }
}, 60000); // Check every minute

// ========================================
// GRACEFUL SHUTDOWN
// ========================================

process.on('SIGINT', async () => {
  console.log('\n\n🛑 Shutting down gracefully...');
  
  if (mqttClient) {
    mqttClient.end();
    console.log('   ✔ MQTT disconnected');
  }
  
  if (mongoClient) {
    await mongoClient.close();
    console.log('   ✔ MongoDB disconnected');
  }
  
  console.log('👋 Goodbye!\n');
  process.exit(0);
});

// ========================================
// START SERVER
// ========================================

async function start() {
  console.log('\n=================================');
  console.log('   BondBot Backend Server');
  console.log('   MQTT ↔ MongoDB ↔ Gemini');
  console.log('=================================\n');
  
  try {
    await connectMongoDB();
    connectMQTT();
    startHTTPServer();
    
    console.log('\n✅ Backend server is running!');
    console.log('   Listening for BondBot messages...\n');
    
  } catch (error) {
    console.error('❌ Failed to start server:', error.message);
    process.exit(1);
  }
}

// Start the server
start();
