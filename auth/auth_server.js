const http = require('http');
const mysql = require('mysql2/promise');
const crypto = require('crypto');
require('dotenv').config();   // loads auth/.env (gitignored). See .env.example.

const config = {
    port: Number(process.env.AUTH_PORT) || 80,
    db: {
        host: process.env.DB_HOST || '127.0.0.1',
        port: Number(process.env.DB_PORT) || 3306,
        user: process.env.DB_USER || 'root',
        password: process.env.DB_PASSWORD || '',
        database: process.env.DB_NAME || 'dreadmyst',
        waitForConnections: true,
        connectionLimit: 10
    }
};

const accounts = new Map([
    [1, { user: 'test', pass: 'test' }],
    [2, { user: 'admin', pass: 'admin' }],
    [3, { user: 'player', pass: 'player' }]
]);

let db;

function log(msg) {
    console.log(`[${new Date().toISOString()}] ${msg}`);
}

function parseCredentials(body) {
    try {
        const decoded = Buffer.from(body, 'base64').toString();
        const parts = decoded.split(':');
        return { user: parts[0], pass: parts[1] || '' };
    } catch (e) {
        const params = new URLSearchParams(body);
        return {
            user: params.get('username') || params.get('user') || '',
            pass: params.get('password') || params.get('pass') || ''
        };
    }
}

function findUser(username, password) {
    for (const [id, creds] of accounts) {
        if (creds.user === username && creds.pass === password) {
            return id;
        }
    }
    return null;
}

async function storeToken(userId, token) {
    const conn = await db.getConnection();
    try {
        await conn.execute('DELETE FROM auth_tokens WHERE user_id = ?', [userId]);
        await conn.execute(
            'INSERT INTO auth_tokens (user_id, token, created_at) VALUES (?, ?, NOW())',
            [userId, token]
        );
    } finally {
        conn.release();
    }
}

function handleAuth(req, res) {
    let data = '';
    
    req.on('data', chunk => data += chunk);
    req.on('end', async () => {
        try {
            const creds = parseCredentials(data);
            const userId = findUser(creds.user, creds.pass);
            
            if (!userId) {
                log(`Auth failed: ${creds.user}`);
                res.writeHead(401, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: 'Invalid credentials' }));
                return;
            }
            
            const token = crypto.randomBytes(16).toString('hex');
            await storeToken(userId, token);
            
            log(`Auth OK: ${creds.user} (id=${userId})`);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: true, token }));
        } catch (err) {
            log(`Error: ${err.message}`);
            res.writeHead(500, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ error: 'Internal error' }));
        }
    });
}

function router(req, res) {
    res.setHeader('Access-Control-Allow-Origin', '*');
    
    if (req.method === 'OPTIONS') {
        res.writeHead(200);
        res.end();
        return;
    }
    
    if (req.method === 'POST' && req.url.startsWith('/auth/')) {
        handleAuth(req, res);
        return;
    }
    
    if (req.method === 'GET' && (req.url === '/' || req.url === '/auth/')) {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('OK');
        return;
    }
    
    res.writeHead(404);
    res.end();
}

async function main() {
    try {
        db = mysql.createPool(config.db);
        await db.query('SELECT 1');
        log('Database connected');
    } catch (err) {
        console.error('DB connection failed:', err.message);
        process.exit(1);
    }
    
    const server = http.createServer(router);
    
    server.on('error', err => {
        if (err.code === 'EACCES') {
            console.error('Port 80 requires admin privileges');
        } else if (err.code === 'EADDRINUSE') {
            console.error('Port already in use');
        } else {
            console.error('Server error:', err.message);
        }
        process.exit(1);
    });
    
    server.listen(config.port, () => {
        log(`Auth server listening on port ${config.port}`);
        console.log('\nAccounts:');
        accounts.forEach((creds, id) => {
            console.log(`  ${id}: ${creds.user}/${creds.pass}`);
        });
        console.log('\nReady.');
    });
}

main();
