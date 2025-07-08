# **Comandos Completos para Deploy e Monitoramento**

## **1. Preparação e Limpeza Inicial**
```powershell
# Limpar tudo primeiro
kubectl delete namespace docker-k8s-lab --force --grace-period=0 2>$null
docker-compose down -v 2>$null
docker stop registry 2>$null
docker rm registry 2>$null

# Configurar contexto Docker Desktop
kubectl config use-context docker-desktop
kubectl config current-context

# Verificar se Kubernetes está funcionando
kubectl get nodes
```

## **2. Build e Deploy**
```powershell
# Navegar para o diretório
cd C:\computer-networks-lab\docker-k8s-lab

# Construir imagem
docker build -t counter-app:latest ./app

# Verificar se a imagem foi criada
docker images | findstr counter-app

# Aplicar todos os manifests
kubectl apply -f k8s/namespace.yaml
kubectl apply -f k8s/redis-deployment.yaml
kubectl apply -f k8s/redis-service.yaml
kubectl apply -f k8s/app-deployment.yaml
kubectl apply -f k8s/app-service.yaml

# Aguardar pods ficarem prontos (máximo 5 minutos)
kubectl wait --for=condition=ready pod --all -n docker-k8s-lab --timeout=300s
```

## **3. Monitoramento de Status**

### **Ver todos os recursos:**
```powershell
# Visão geral completa
kubectl get all -n docker-k8s-lab

# Pods com mais detalhes
kubectl get pods -n docker-k8s-lab -o wide

# Services
kubectl get services -n docker-k8s-lab

# Deployments
kubectl get deployments -n docker-k8s-lab
```

### **Status dos pods em tempo real:**
```powershell
# Watch dos pods (atualiza automaticamente)
kubectl get pods -n docker-k8s-lab -w

# Eventos do namespace
kubectl get events -n docker-k8s-lab --sort-by='.lastTimestamp'

# Status detalhado de um pod específico
kubectl describe pod -l app=counter-app -n docker-k8s-lab
```

### **Logs dos serviços:**
```powershell
# Logs da aplicação (todos os pods)
kubectl logs -l app=counter-app -n docker-k8s-lab -f

# Logs do Redis
kubectl logs -l app=redis -n docker-k8s-lab -f

# Logs de um pod específico
kubectl logs <nome-do-pod> -n docker-k8s-lab -f
```

## **4. Testar a Aplicação**

### **Port-forward para acesso:**
```powershell
# Abrir túnel para a aplicação (deixar rodando)
kubectl port-forward service/counter-app-service 8080:80 -n docker-k8s-lab
```

### **Em outro terminal, testar endpoints:**
```powershell
# Endpoint principal
Invoke-RestMethod -Uri http://localhost:8080/ -Method Get

# Contador (fazer várias vezes para ver load balancing)
Invoke-RestMethod -Uri http://localhost:8080/count -Method Get
Invoke-RestMethod -Uri http://localhost:8080/count -Method Get
Invoke-RestMethod -Uri http://localhost:8080/count -Method Get

# Estatísticas
Invoke-RestMethod -Uri http://localhost:8080/stats -Method Get

# Health check
Invoke-RestMethod -Uri http://localhost:8080/health -Method Get
```

### **Teste automatizado de load balancing:**
```powershell
# Fazer 20 requisições para ver distribuição
for ($i=1; $i -le 20; $i++) {
    $response = Invoke-RestMethod -Uri http://localhost:8080/count -Method Get
    Write-Host "Requisição $i - Pod: $($response.handled_by) - Total: $($response.total_requests)"
    Start-Sleep -Seconds 1
}
```

## **5. Demonstrar Escalabilidade**

### **Escalar aplicação:**
```powershell
# Escalar para 5 pods
kubectl scale deployment counter-app-deployment --replicas=5 -n docker-k8s-lab

# Ver pods sendo criados
kubectl get pods -n docker-k8s-lab -w

# Aguardar todos ficarem prontos
kubectl wait --for=condition=ready pod --all -n docker-k8s-lab --timeout=300s

# Verificar distribuição após escalar
for ($i=1; $i -le 10; $i++) {
    $response = Invoke-RestMethod -Uri http://localhost:8080/count -Method Get
    Write-Host "Pod: $($response.handled_by)"
}
```

### **Simular falha de pod:**
```powershell
# Listar pods
kubectl get pods -n docker-k8s-lab

# Deletar um pod específico
kubectl delete pod <nome-do-pod> -n docker-k8s-lab

# Ver Kubernetes recriando automaticamente
kubectl get pods -n docker-k8s-lab -w

# Testar se aplicação continua funcionando
Invoke-RestMethod -Uri http://localhost:8080/count -Method Get
```

## **6. Monitoramento Avançado**

### **Comandos de debugging:**
```powershell
# Ver recursos do cluster
kubectl top nodes
kubectl top pods -n docker-k8s-lab

# Entrar dentro de um pod
kubectl exec -it <nome-do-pod> -n docker-k8s-lab -- /bin/bash

# Verificar conectividade entre pods
kubectl exec -it <nome-do-pod> -n docker-k8s-lab -- curl redis-service:6379

# Ver configuração do deployment
kubectl describe deployment counter-app-deployment -n docker-k8s-lab
```

### **Status dos health checks:**
```powershell
# Ver status das probes
kubectl describe pod -l app=counter-app -n docker-k8s-lab | findstr -i "liveness\|readiness"

# Testar health check diretamente
kubectl exec -it <nome-do-pod> -n docker-k8s-lab -- curl localhost:5000/health
```

## **7. Script Completo de Demonstração**

```powershell
# Script para demonstração completa
Write-Host "=== INICIANDO DEPLOY ===" -ForegroundColor Green
kubectl apply -f k8s/
kubectl wait --for=condition=ready pod --all -n docker-k8s-lab --timeout=300s

Write-Host "=== STATUS DOS PODS ===" -ForegroundColor Yellow
kubectl get pods -n docker-k8s-lab -o wide

Write-Host "=== TESTANDO APLICAÇÃO ===" -ForegroundColor Cyan
Start-Process -FilePath "kubectl" -ArgumentList "port-forward service/counter-app-service 8080:80 -n docker-k8s-lab" -WindowStyle Hidden
Start-Sleep -Seconds 5

for ($i=1; $i -le 5; $i++) {
    $response = Invoke-RestMethod -Uri http://localhost:8080/count -Method Get
    Write-Host "Requisição $i - Pod: $($response.handled_by) - Total: $($response.total_requests)" -ForegroundColor White
}

Write-Host "=== ESTATÍSTICAS FINAIS ===" -ForegroundColor Magenta
$stats = Invoke-RestMethod -Uri http://localhost:8080/stats -Method Get
$stats | ConvertTo-Json -Depth 3
```

## **8. Limpeza Final**
```powershell
# Parar port-forward (Ctrl+C no terminal do port-forward)

# Deletar namespace (remove tudo)
kubectl delete namespace docker-k8s-lab

# Verificar limpeza
kubectl get all -n docker-k8s-lab
```

## **Ordem de Execução para Apresentação:**

1. **Execute seção 1**: Preparação
2. **Execute seção 2**: Deploy 
3. **Execute seção 3**: Monitoramento básico
4. **Execute seção 4**: Testes da aplicação
5. **Execute seção 5**: Demonstrar escalabilidade
6. **Execute seção 6**: Mostrar recursos avançados
7. **Execute seção 8**: Limpeza

**Dica**: Tenha **dois terminais abertos** - um para port-forward e outro para comandos de teste!