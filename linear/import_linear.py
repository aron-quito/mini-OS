#!/usr/bin/env python3
"""
Automatización de creación de Proyectos (Módulos), Milestones, Tareas (Issues)
y Dependencias en Linear vía GraphQL API.

Estructura:
- Projects = Módulos de MiniOS (Shell Base, Procesos, Hilos, Memoria, Integración)
- Milestones = Hitos dentro de cada Proyecto/Módulo
- Issues = Tareas asignadas a su Proyecto y Milestone específico
- Dependencies = Relaciones de bloqueo (blocks / is blocked by) inter-issues
"""

import os
import sys
import json
import urllib.request
import urllib.error

LINEAR_API_URL = "https://api.linear.app/graphql"
LINEAR_API_KEY = os.environ.get("LINEAR_API_KEY", "").strip()

if not LINEAR_API_KEY:
    try:
        LINEAR_API_KEY = input("🔑 Ingrese su Personal API Key de Linear (o exporte LINEAR_API_KEY): ").strip()
    except (EOFError, KeyboardInterrupt):
        pass

if not LINEAR_API_KEY:
    print("[!] Error: No se proporcionó la API Key de Linear.")
    print("    Por favor defina la variable de entorno: export LINEAR_API_KEY='tu_api_key'")
    sys.exit(1)

TEAM_KEY = os.environ.get("LINEAR_TEAM_KEY", "SIS")

MODULE_DESCRIPTIONS = {
    "Módulo 0 — Shell & Infraestructura Base": (
        "Arquitectura modular del mini-OS: REPL interactivo (miniOS>), "
        "tokenizador de comandos, dispatcher central y comandos built-in (help, clear, exit)."
    ),
    "Módulo 1 — Gestión de Procesos y Concurrencia": (
        "Gestión de procesos mediante llamadas al sistema Linux (fork, execvp, waitpid), "
        "ejecución en background (&) con reserva de memoria simulada y comando ps-mini."
    ),
    "Módulo 2 — Gestión de Hilos (Pthreads & Concurrencia)": (
        "Concurrencia con POSIX Threads (pthreads), sincronización thread-safe "
        "mediante mutex y variables de condición, búfer circular y validación de 0 pérdidas."
    ),
    "Módulo 3 — Gestión de Memoria (First-Fit & Paginación)": (
        "Simulación integral de RAM: particionamiento dinámico First-Fit con coalescencia, "
        "cola FIFO de espera por RAM insuficiente (mem_queue), paginación simple y visualizador interactivo."
    ),
    "Integración Global & Entregables Finales": (
        "Hardening del sistema completo, manejo de señales POSIX con sigaction, "
        "batería de pruebas automatizadas, auditoría Valgrind/Helgrind y entregables académicos."
    )
}

def execute_graphql(query: str, variables: dict = None) -> dict:
    payload = json.dumps({"query": query, "variables": variables or {}}).encode("utf-8")
    req = urllib.request.Request(
        LINEAR_API_URL,
        data=payload,
        headers={
            "Authorization": LINEAR_API_KEY,
            "Content-Type": "application/json"
        }
    )
    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            if "errors" in data:
                print(f"[!] Error de GraphQL: {json.dumps(data['errors'], indent=2)}")
                sys.exit(1)
            return data["data"]
    except urllib.error.HTTPError as err:
        body = err.read().decode("utf-8")
        print(f"[!] HTTP Error {err.code}: {body}")
        sys.exit(1)

def get_team_id(team_key: str) -> str:
    query = """
    query GetTeams {
      teams {
        nodes {
          id
          key
          name
        }
      }
    }
    """
    res = execute_graphql(query)
    for team in res["teams"]["nodes"]:
        if team["key"] == team_key:
            print(f"[+] Equipo encontrado: {team['name']} ({team['key']}) -> ID: {team['id']}")
            return team["id"]
    print(f"[!] Equipo con clave '{team_key}' no encontrado.")
    sys.exit(1)

def get_or_create_project(team_id: str, project_name: str, description: str) -> tuple[str, str]:
    query_existing = """
    query GetProjects {
      projects {
        nodes {
          id
          name
          url
        }
      }
    }
    """
    res = execute_graphql(query_existing)
    for p in res["projects"]["nodes"]:
        if p["name"] == project_name:
            print(f"  [=] Proyecto existente: '{project_name}' -> ID: {p['id']}")
            return p["id"], p["url"]

    create_mutation = """
    mutation CreateProject($input: ProjectCreateInput!) {
      projectCreate(input: $input) {
        success
        project {
          id
          name
          url
        }
      }
    }
    """
    input_data = {
        "name": project_name,
        "teamIds": [team_id],
        "description": description
    }
    result = execute_graphql(create_mutation, {"input": input_data})
    proj = result["projectCreate"]["project"]
    print(f"  [+] Creado Proyecto: '{proj['name']}' -> ID: {proj['id']}")
    return proj["id"], proj["url"]

def get_or_create_milestone(project_id: str, milestone_name: str) -> str:
    query_milestones = """
    query GetProjectMilestones($projectId: String!) {
      project(id: $projectId) {
        projectMilestones {
          nodes {
            id
            name
          }
        }
      }
    }
    """
    res = execute_graphql(query_milestones, {"projectId": project_id})
    existing = res.get("project", {}).get("projectMilestones", {}).get("nodes", [])
    for m in existing:
        if m["name"] == milestone_name:
            print(f"    [=] Milestone existente: '{milestone_name}' -> ID: {m['id']}")
            return m["id"]

    create_mutation = """
    mutation CreateMilestone($input: ProjectMilestoneCreateInput!) {
      projectMilestoneCreate(input: $input) {
        success
        projectMilestone {
          id
          name
        }
      }
    }
    """
    input_data = {
        "projectId": project_id,
        "name": milestone_name
    }
    result = execute_graphql(create_mutation, {"input": input_data})
    milestone = result["projectMilestoneCreate"]["projectMilestone"]
    print(f"    [+] Creado Milestone: '{milestone['name']}' -> ID: {milestone['id']}")
    return milestone["id"]

def create_issue(team_id: str, project_id: str, milestone_id: str, title: str, description: str) -> tuple[str, str, str]:
    mutation = """
    mutation CreateIssue($input: IssueCreateInput!) {
      issueCreate(input: $input) {
        success
        issue {
          id
          identifier
          title
          url
        }
      }
    }
    """
    input_data = {
        "teamId": team_id,
        "projectId": project_id,
        "projectMilestoneId": milestone_id,
        "title": title,
        "description": description
    }
    result = execute_graphql(mutation, {"input": input_data})
    issue = result["issueCreate"]["issue"]
    return issue["id"], issue["identifier"], issue["url"]

def create_issue_relation(blocker_id: str, blocked_id: str) -> bool:
    mutation = """
    mutation CreateIssueRelation($input: IssueRelationCreateInput!) {
      issueRelationCreate(input: $input) {
        success
      }
    }
    """
    input_data = {
        "issueId": blocker_id,
        "relatedIssueId": blocked_id,
        "type": "blocks"
    }
    result = execute_graphql(mutation, {"input": input_data})
    return result["issueRelationCreate"]["success"]

def main():
    print("=" * 80)
    print(" 🚀 LINEAR IMPORT: Proyectos como Módulos + Milestones + Issues + Dependencias")
    print("=" * 80)

    json_path = os.path.join(os.path.dirname(__file__), "linear_tasks.json")
    if not os.path.exists(json_path):
        print(f"[!] No se encontró el archivo de tareas en: {json_path}")
        sys.exit(1)

    with open(json_path, "r", encoding="utf-8") as f:
        tasks = json.load(f)

    print(f"[i] Se cargaron {len(tasks)} tareas desde {json_path}")

    # 1. Obtener Team ID
    team_id = get_team_id(TEAM_KEY)

    # 2. Identificar y crear Proyectos (Módulos)
    print("\n📂 Paso 1: Configurando Proyectos como Módulos en Linear...")
    projects_cache = {}  # project_name -> { "id": id, "url": url }
    unique_projects = list(dict.fromkeys(t["project"] for t in tasks))

    for proj_name in unique_projects:
        desc = MODULE_DESCRIPTIONS.get(proj_name, f"Módulo de desarrollo para {proj_name}")
        p_id, p_url = get_or_create_project(team_id, proj_name, desc)
        projects_cache[proj_name] = {"id": p_id, "url": p_url}

    # 3. Crear Milestones dentro de cada Proyecto
    print("\n🚩 Paso 2: Creando Milestones específicos dentro de cada Proyecto...")
    milestones_cache = {}  # (project_name, milestone_name) -> milestone_id
    project_milestone_pairs = list(dict.fromkeys((t["project"], t["milestone"]) for t in tasks))

    for proj_name, m_name in project_milestone_pairs:
        p_id = projects_cache[proj_name]["id"]
        m_id = get_or_create_milestone(p_id, m_name)
        milestones_cache[(proj_name, m_name)] = m_id

    # 4. Crear los Issues vinculados a su Proyecto y Milestone
    print("\n📝 Paso 3: Creando Tareas (Issues) en Linear...")
    created_issues = {}  # task_id -> { "linear_id": id, "identifier": "SIS-X", "title": ... }

    for i, t in enumerate(tasks, 1):
        p_id = projects_cache[t["project"]]["id"]
        m_id = milestones_cache[(t["project"], t["milestone"])]

        # Enriquecer descripción con detalles de dependencias
        deps_text = ", ".join(t["blocked_by"]) if t["blocked_by"] else "Ninguna (Punto de partida)"
        full_desc = f"{t['description']}\n\n**🔗 Depende de:** {deps_text}\n**📦 Módulo (Proyecto):** {t['project']}\n**🚩 Milestone:** {t['milestone']}"

        issue_id, identifier, issue_url = create_issue(
            team_id=team_id,
            project_id=p_id,
            milestone_id=m_id,
            title=t["title"],
            description=full_desc
        )
        created_issues[t["id"]] = {
            "linear_id": issue_id,
            "identifier": identifier,
            "title": t["title"],
            "url": issue_url
        }
        print(f"  [{i}/{len(tasks)}] {identifier}: {t['title']} (Módulo: {t['project']})")

    # 5. Establecer dependencias entre tareas
    print("\n🔗 Paso 4: Creando relaciones de dependencia ('blocks')...")
    relations_count = 0
    for t in tasks:
        blocked_task_id = t["id"]
        blocked_linear = created_issues[blocked_task_id]

        for blocker_task_id in t["blocked_by"]:
            if blocker_task_id in created_issues:
                blocker_linear = created_issues[blocker_task_id]
                success = create_issue_relation(
                    blocker_id=blocker_linear["linear_id"],
                    blocked_id=blocked_linear["linear_id"]
                )
                if success:
                    relations_count += 1
                    print(f"  🔒 {blocker_linear['identifier']} bloquea a -> {blocked_linear['identifier']}")
            else:
                print(f"  [!] Advertencia: Dependencia '{blocker_task_id}' no encontrada en mapa.")

    # 6. Resumen final
    print("\n" + "=" * 80)
    print(" 🎉 PROCESO COMPLETADO EXITOSAMENTE")
    print("=" * 80)
    print(f"• Proyectos / Módulos creados : {len(projects_cache)}")
    print(f"• Milestones creados          : {len(milestones_cache)}")
    print(f"• Issues creados              : {len(created_issues)}")
    print(f"• Dependencias configuradas   : {relations_count}")
    print("\n📋 Enlaces a los Proyectos (Módulos) en Linear:")
    for name, pdata in projects_cache.items():
        print(f"  - {name}: {pdata['url']}")
    print("=" * 80)

if __name__ == "__main__":
    main()
