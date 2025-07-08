from flask import Flask, jsonify, request
import redis
import os
import socket
import json
from datetime import datetime

app = Flask(__name__)

# Configuração do Redis
redis_host = os.getenv('REDIS_HOST', 'localhost')
redis_port = int(os.getenv('REDIS_PORT', 6379))
redis_client = redis.Redis(host=redis_host, port=redis_port, decode_responses=True)

# Nome do container para identificação
container_name = os.getenv('HOSTNAME', socket.gethostname())

@app.route('/')
def home():
    return jsonify({
        "message": "API de Contagem - Docker + Kubernetes Lab",
        "container": container_name,
        "endpoints": [
            "/count - Incrementa e retorna contador",
            "/stats - Estatísticas detalhadas",
            "/health - Health check"
        ]
    })

@app.route('/count', methods=['GET', 'POST'])
def count():
    try:
        # Incrementa contador global
        total_count = redis_client.incr('total_requests')
        
        # Incrementa contador por container
        container_count = redis_client.incr(f'container:{container_name}')
        
        # Registra timestamp
        redis_client.lpush('requests_log', json.dumps({
            'timestamp': datetime.now().isoformat(),
            'container': container_name,
            'total': total_count,
            'container_count': container_count
        }))
        
        # Mantém apenas os últimos 100 logs
        redis_client.ltrim('requests_log', 0, 99)
        
        return jsonify({
            "total_requests": total_count,
            "container_requests": container_count,
            "handled_by": container_name,
            "timestamp": datetime.now().isoformat()
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/stats')
def stats():
    try:
        # Estatísticas gerais
        total_requests = redis_client.get('total_requests') or 0
        
        # Estatísticas por container
        container_stats = {}
        for key in redis_client.keys('container:*'):
            container_name_key = key.replace('container:', '')
            container_stats[container_name_key] = redis_client.get(key)
        
        # Últimas requisições
        recent_logs = redis_client.lrange('requests_log', 0, 9)
        recent_requests = [json.loads(log) for log in recent_logs]
        
        return jsonify({
            "total_requests": int(total_requests),
            "container_stats": container_stats,
            "recent_requests": recent_requests,
            "active_containers": len(container_stats)
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/health')
def health():
    try:
        # Testa conexão com Redis
        redis_client.ping()
        return jsonify({
            "status": "healthy",
            "container": container_name,
            "redis_connection": "ok"
        })
    except:
        return jsonify({
            "status": "unhealthy",
            "container": container_name,
            "redis_connection": "failed"
        }), 503

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)